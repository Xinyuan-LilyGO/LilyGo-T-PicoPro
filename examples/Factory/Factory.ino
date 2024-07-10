/**
 * @file      lvgl.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-13
 *
 */

#include <stdio.h>
#include <TFT_eSPI.h>           // https://github.com/Bodmer/TFT_eSPI
#include <WiFiEspAT.h>          // https://github.com/jandrassy/WiFiEspAT
#include <lvgl.h>               // https://github.com/lvgl/lvgl
#include <time.h>
#include <XPowersLib.h>         // https://github.com/lewisxhe/XPowersLib
#include <TouchDrvCSTXXX.hpp>   // https://github.com/lewisxhe/SensorsLib
#include <SensorWireHelper.h>   // https://github.com/lewisxhe/SensorsLib
#include <SensorLTR553.hpp>     // https://github.com/lewisxhe/SensorsLib
#include <AceButton.h>
#include <SD.h>
#include "utilities.h"
#include "gui.h"
#include "esp_gpio.h"

using namespace ace_button;

#if (BOARD_SPI_MISO != TFT_MISO)    \
    || (BOARD_SPI_MOSI != TFT_MOSI) \
    || (BOARD_SPI_SCK!= TFT_SCLK)  \
    || (TFT_CS!= BOARD_TFT_CS)      \
    || (TFT_DC!= BOARD_TFT_DC)      \
    || (TFT_RST!= BOARD_TFT_RST)
#error  "Error! Please make sure <Setup212_LilyGo_T_PicoPro.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <Setup212_LilyGo_T_PicoPro.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <Setup212_LilyGo_T_PicoPro.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#error  "Error! Please make sure <Setup212_LilyGo_T_PicoPro.h> is selected in <TFT_eSPI/User_Setup_Select.h>"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID             "Your WiFi SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD         "Your WiFi PASSWORD"
#endif

const uint8_t btns[BOARD_USER_BTN_NUM] = BOARD_USER_BUTTON;

TFT_eSPI tft = TFT_eSPI();
SensorLTR553 als;
PowersSY6970 PMU;
TouchDrvCSTXXX touch;

AceButton   btn1(/*PIN*/btns[0],/*DEFAULT STATE*/  HIGH,/*ID*/  0);
AceButton   btn2(/*PIN*/btns[1],/*DEFAULT STATE*/  HIGH,/*ID*/  1);

#define TIME_ZONE           8       // UTC + 8

static void showDevicesProbe();
static bool syncDateTime();

static bool hasPMU, hasALS, hasTouch, hasSD, hasWiFi;
static bool isBacklightOn = true;
static bool manualOff = false;
static lv_disp_draw_buf_t disp_buf;  // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;       // contains callback functions
static bool isActive = false;
static bool isUpdateWiFi = false;
static uint32_t checkInterval;

// Full buffering will result in insufficient memory
// #define LV_BUFFER_SIZE      TFT_HEIGHT * TFT_WIDTH  * sizeof(lv_color16_t)

#define LV_BUFFER_SIZE      TFT_HEIGHT * TFT_WIDTH / 2
#ifdef USING_MALLOC_FUNC
static lv_color_t *lv_disp_buf;
#else
static lv_color_t lv_disp_buf[LV_BUFFER_SIZE];
#endif
static void handleEvent(AceButton *button, uint8_t eventType, uint8_t buttonState);

String macAddress(void)
{
    uint8_t mac[6];
    char macStr[18] = { 0 };
    WiFi.macAddress(mac);
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String getModel()
{
    SerialAT.println("AT+GMR");
    String str = SerialAT.readString();
    int idx = str.indexOf("ESP");
    if (idx == -1) {
        return "None";
    }
    str = str.substring(idx, str.indexOf(" ", idx));
    return str;
}

static void lvgl_flush_callback(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
    tft.endWrite();
    lv_disp_flush_ready( drv );
}

static void lv_touchpad_read_callback(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int16_t x[5], y[5];
    if (touch.getPoint(x, y, 1)) {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum brightness
void setBrightness(uint8_t value)
{
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        digitalWrite(BOARD_TFT_BL, 0);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(BOARD_TFT_BL, 1);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BOARD_TFT_BL, 0);
        digitalWrite(BOARD_TFT_BL, 1);
    }
    level = value;
}


void setup()
{
    Serial.begin(115200);
    // while (!Serial);

    Wire.setSDA(BOARD_I2C_SDA);
    Wire.setSCL(BOARD_I2C_SCL);
    Wire.begin();

    /* Initialize the PMU*/
    Serial.println("init PMU .");
    hasPMU = PMU.begin(Wire, SY6970_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (!hasPMU) {
        Serial.println("PMU is not online!"); delay(500);
    } else {
        Serial.println("Find PMU.");

        // Set USB input current limit
        PMU.setInputCurrentLimit(1000);

        // In LilyGo products, the OTG external enable Pin is connected to
        // VSYS by default and is closed after the initialization is completed.
        PMU.disableOTG();

        PMU.setChargeTargetVoltage(4208);   //4.208V   ,step:16 mV
        PMU.setChargerConstantCurr(320);    //320mA  , 6step:64 mA
        PMU.setPrechargeCurr(192);          //Range: 64mA ~ 1024mA ,step:64mA
        PMU.enableADCMeasure();
        Serial.printf("getSysPowerDownVoltage:%u\n", PMU.getSysPowerDownVoltage());
        Serial.printf("getChargeTargetVoltage:%u\n", PMU.getChargeTargetVoltage());
        Serial.printf("getChargerConstantCurr:%u\n", PMU.getChargerConstantCurr());
        Serial.printf("getPrechargeCurr:%u\n", PMU.getPrechargeCurr());


        // For devices that have been connected to the battery, the charging function is turned on by default.
        // PMU.enableCharge();

        // For boards without an external battery, the charging function should be turned off, otherwise the power supply current of the power chip will be limited.
        PMU.disableCharge();

    }

    // Dump I2C Devices
    // SensorWireHelper::dumpDevices(Wire, Serial);

    /* Initialize the Proximity sensor */
    Serial.println("init ALS .");
    hasALS = als.begin(Wire, LTR553_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (!hasALS) {
        Serial.println("Failed to find Proximity & Light sensor !");
        delay(500);
    } else {
        Serial.println("Find Proximity & Light sensor");
        // Enable light sensor
        als.enableLightSensor();

        // Enable proximity sensor
        als.enableProximity();
    }


    Wire1.setSDA(QWIIC_I2C_SDA);
    Wire1.setSCL(QWIIC_I2C_SCL);
    Wire1.begin();
    // Dump I2C Devices
    // SensorWireHelper::dumpDevices(Wire1, Serial);

    // Share SPI Bus , disable bus spi devices
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);

    uint32_t  freq = clock_get_hz(clk_sys);
    Serial.printf("CPU Freq: %lu MHz\n", freq / 1000 / 1000);

    /* Initialize the sd card */
    SPI.setRX(BOARD_SPI_MISO);
    SPI.setTX(BOARD_SPI_MOSI);
    SPI.setSCK(BOARD_SPI_SCK);

    // see if the card is present and can be initialized:
    hasSD = SD.begin(BOARD_SD_CS);
    if (!hasSD) {
        Serial.println("Card failed, or not present");
    } else {
        Serial.print("SD initialization done.");
        Serial.print(SD.size64() / 1024 / 1024.0);
        Serial.println("MB");
    }

    /* Initialize the screen */
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Serial.println("init Touch .");
    touch.setPins(BOARD_TOUCH_IRQ, BOARD_TOUCH_IRQ);
    hasTouch = touch.begin(Wire, CST226SE_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL);
    if (!hasPMU) {
        Serial.println("Touch is not online!"); delay(500);
    } else {
        Serial.println("Find Touch.");
        touch.setMaxCoordinates(tft.width(), tft.height());
        touch.setSwapXY(true);
        touch.setMirrorXY(true, false);

        touch.setHomeButtonCallback([](void *user_data) {
            Serial.println("Home key pressed!");
            static uint32_t checkMs = 0;
            if (millis() > checkMs) {
                if (isBacklightOn) {
                    for (int i = 16; i >= 0; --i) {
                        setBrightness( i);
                        delay(10);
                    }
                    Serial.println("turn off!");
                    manualOff = true;
                    isBacklightOn = false;
                } else {
                    manualOff = false;
                    isBacklightOn = true;
                    for (int i = 0; i <= 16; ++i) {
                        setBrightness( i);
                        delay(10);
                    }
                    Serial.println("turn on!");
                }
            }
            checkMs = millis() + 200;
        }, NULL);

    }


    Serial.println("init WiFi .");
    /* Configure pins for communication with ESP-AT */
    SerialAT.setRX(BOARD_WIFI_RX);
    SerialAT.setTX(BOARD_WIFI_TX);
    SerialAT.begin(115200);

    hasWiFi = WiFi.init(SerialAT, BOARD_WIFI_EN);
    if (!hasWiFi) {
        Serial.println("WiFi Module is not online !");
        delay(500);
    }
    for (int i = 0; i <= 16; ++i) {
        setBrightness( i);
        delay(3);
    }


#ifdef USING_MALLOC_FUNC
    lv_disp_buf = (lv_color16_t *)malloc(LV_BUFFER_SIZE);
    if (!lv_disp_buf) {
        while (1) {
            Serial.println("no memory !!!!");
            delay(1000);
        }
    }
#endif
    Serial.println("init lvgl");

    lv_init();
    lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, LV_BUFFER_SIZE);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width();
    disp_drv.ver_res = tft.height();
    disp_drv.flush_cb = lvgl_flush_callback;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register touch brush with LVGL */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_touchpad_read_callback;
    lv_indev_drv_register(&indev_drv);

    showDevicesProbe();

    Serial.println("init UI");
    initGUI();

    if (hasWiFi) {


        WiFi.disconnect();     // to clear the way. not persistent
        WiFi.setPersistent();  // set the following WiFi connection as persistent
        WiFi.endAP();          // to disable default automatic start of persistent AP at startup

        Serial.print("FW Version:");
        Serial.println(WiFi.firmwareVersion());
        Serial.print("MAC: " + macAddress());

        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


        while (!testAT()) {
            delay(1);
        }
        Serial.println("Init ESP GPIO");
        // Set all gpio input output mode
        // 0x0:IN, 0x1:OUTPUT, 0x2:INPUT or OUTPUT, 0x3:DISABLE
        for (int i = 0; i < EIO_MAX; ++i) {
            esp_pin_mode(i, 0x02);
        }
    }

    ButtonConfig *buttonConfig = ButtonConfig::getSystemButtonConfig();
    buttonConfig->setEventHandler(handleEvent);
}

void loop()
{
    if (BOOTSEL) {
        Serial.printf("You pressed BOOTSEL \n");
    }

    btn1.check();
    btn2.check();

    if (hasWiFi && !isActive && millis() > checkInterval) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!isUpdateWiFi) {
                IPAddress ip = WiFi.localIP();
                Serial.print("IP Address: ");
                Serial.println(ip);
                lv_msg_send(LV_MSG_WIFI_GET_IP, (const char *)ip.toString().c_str());
                //Enable sntp
                WiFi.sntp("cn.pool.ntp.org", "ntp.sjtu.edu.cn");
                isUpdateWiFi = true;
            } else if (syncDateTime()) {
                isActive = true;
                Serial.println("Update datetime done!");
            }
        }
        checkInterval = millis() + 3000;
    }
    lv_task_handler();
    delay(1);
}

static void showDevicesProbe()
{
    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_remove_style(cont, 0, LV_PART_SCROLLBAR);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, LV_PART_MAIN);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_center(cont);

    serialToScreen(cont, "Power management", hasPMU);
    serialToScreen(cont, "Capacitive Touch", hasTouch);
    serialToScreen(cont, "Proximity & Light sensor", hasALS);
    if (hasSD) {
        serialToScreen(cont, "Mass storage #FFFFFF [# #00ff00  "
                       + String(SD.size64() / 1024 / 1024.0 )
                       + "MB# #FFFFFF ]#", true);
    } else {
        serialToScreen(cont, "Mass storage", false);
    }

    if (hasWiFi) {
        serialToScreen(cont, "WiFi Module #FFFFFF [# #00ff00  " +
                       getModel() + " V" + String(WiFi.firmwareVersion())
                       + "# #FFFFFF ]#", true);
    } else {
        serialToScreen(cont, "WiFi Module", false);
    }


    uint32_t endTime = millis() + 5000;
    while (millis() < endTime) {
        lv_task_handler();
        delay(1);
    }
    lv_obj_del(cont);
}

void sensor_timer_callback(lv_timer_t *t)
{
    if (hasALS) {
        bool saturated;
        uint16_t ps = als.getProximity(&saturated);
        lv_label_set_text_fmt((lv_obj_t * )t->user_data, "%u",  ps);
    }
}


// https://docs.espressif.com/projects/esp-at/zh_CN/latest/esp32c3/AT_Command_Set/TCP-IP_AT_Commands.html
static bool syncDateTime()
{
    struct tm t_tm;
    char buffer[256];
    SerialAT.println("AT+CIPSNTPTIME?");
    if (SerialAT.readBytesUntil(':', buffer, 256)) {
        String str = SerialAT.readStringUntil('\n');
        Serial.println(str);
        char week[10];
        char imonth[10];

        sscanf(str.c_str(), "%s %s %d %d:%d:%d %d",
               week,
               imonth,
               &t_tm.tm_mday,
               &t_tm.tm_hour,
               &t_tm.tm_min,
               &t_tm.tm_sec,
               &t_tm.tm_year);

        switch (imonth[0]) {
        case 'J':
            if ( imonth[1] == 'a' )
                t_tm.tm_mon = 1;
            else if ( imonth[2] == 'n' )
                t_tm.tm_mon = 6;
            else
                t_tm.tm_mon = 7;
            break;
        case 'F':
            t_tm.tm_mon = 2;
            break;
        case 'A':
            t_tm.tm_mon = imonth[1] == 'p' ? 4 : 8;
            break;
        case 'M':
            t_tm.tm_mon = imonth[2] == 'r' ? 3 : 5;
            break;
        case 'S':
            t_tm.tm_mon = 9;
            break;
        case 'O':
            t_tm.tm_mon = 10;
            break;
        case 'N':
            t_tm.tm_mon = 11;
            break;
        case 'D':
            t_tm.tm_mon = 12;
            break;
        }
        if (t_tm.tm_year > 2000) {
            t_tm.tm_year -= 1900;
            t_tm.tm_mon -= 1;
            struct timeval val;
            val.tv_sec = mktime(&t_tm) + (TIME_ZONE * 3600);
            val.tv_usec = 0;
            settimeofday(&val, NULL);
            return true;
        }
    }
    return false;
}


// The event handler for the button.
static void handleEvent(AceButton *button, uint8_t eventType, uint8_t buttonState)
{
    // Print out a message for all events.
    Serial.print(F("handleEvent(): eventType: "));
    Serial.print(AceButton::eventName(eventType));
    Serial.print(F("; buttonState: "));
    Serial.println(buttonState);
    uint8_t id = button->getId();
    uint8_t r = tft.getRotation();
    switch (eventType) {
    case AceButton::kEventPressed:
        Serial.printf("ID:%d is Pressed!", id);
        if (r == 3) {
            tft.setRotation(1);
            lv_disp_drv_update(lv_disp_get_default(), &disp_drv);
            touch.setMirrorXY(false, true);
        } else if (r == 1) {
            tft.setRotation(3);
            lv_disp_drv_update(lv_disp_get_default(), &disp_drv);
            touch.setMirrorXY(true, false);
        }
        break;
    default:
        break;
    }
}
