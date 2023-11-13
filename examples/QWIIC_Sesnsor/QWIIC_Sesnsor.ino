/**
 * @file      QWIIC_Sesnsor.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-11-09
 * @note      As a Qwiic interface demonstration, the sketch only demonstrates the MPU9250 and BME280 sensors
 */

#include <Wire.h>
#include <TFT_eSPI.h>           // https://github.com/Bodmer/TFT_eSPI
#include "utilities.h"
#define SEALEVELPRESSURE_HPA (1013.25)
#include <lvgl.h>               // https://github.com/lvgl/lvgl
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

#include <MPU9250.h>
MPU9250 IMU(Wire1, 0x68);
uint32_t measurementInterval = 0;
lv_obj_t *sensor_label;
TFT_eSPI tft = TFT_eSPI();


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


#define LV_BUFFER_SIZE      TFT_HEIGHT * TFT_WIDTH / 2
static lv_color_t lv_disp_buf[LV_BUFFER_SIZE];



bool sensorOnline_BME280 = false;
bool sensorOnline_MPU9250 = false;
uint8_t sensor_addr_BMX280 = 0x77;
static void lvglHelper();
static void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

void setup()
{
    Serial.begin(115200);

    Wire1.setSDA(QWIIC_I2C_SDA);
    Wire1.setSCL(QWIIC_I2C_SCL);
    Wire1.begin();

    lvglHelper();

    // default settings
    sensorOnline_BME280 = bme.begin(0x77, &Wire1);
    if (sensorOnline_BME280) {
        bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                        Adafruit_BME280::SAMPLING_X1, // temperature
                        Adafruit_BME280::SAMPLING_X1, // pressure
                        Adafruit_BME280::SAMPLING_X1, // humidity
                        Adafruit_BME280::FILTER_X2   );
    }

    sensorOnline_MPU9250 = IMU.begin();

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_size(cont, lv_disp_get_physical_hor_res(NULL), lv_disp_get_physical_ver_res(NULL));
    lv_obj_remove_style(cont, 0, LV_PART_SCROLLBAR);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);


    sensor_label = lv_label_create(cont);
    lv_obj_set_style_text_color(sensor_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(sensor_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_label_set_text(sensor_label, "No Sensor detecte!");
}

void loop()
{
    char buf[256] = {0};

    if (sensorOnline_BME280) {
        if ( millis() > measurementInterval ) {
            measurementInterval = millis() + 1000;

            float temp = bme.readTemperature();
            float press = bme.readPressure() / 100.0F;
            float humi = bme.readHumidity();


            snprintf(buf, 256, "Temperature:%.2f\n\nPressure:%.2f\n\nHumidity:%.2f", temp, press, humi);

            lv_label_set_text(sensor_label, buf);

        }
    }

    if (sensorOnline_MPU9250) {
        if ( millis() > measurementInterval ) {
            measurementInterval = millis() + 200;
            // read the sensor
            IMU.readSensor();

            // display the data
            // Serial.print(IMU.getAccelX_mss(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getAccelY_mss(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getAccelZ_mss(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getGyroX_rads(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getGyroY_rads(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getGyroZ_rads(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getMagX_uT(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getMagY_uT(), 6);
            // Serial.print("\t");
            // Serial.print(IMU.getMagZ_uT(), 6);
            // Serial.print("\t");
            // Serial.println(IMU.getTemperature_C(), 6);

            //Just show mag value
            snprintf(buf, 256, "Temperature:%.2f\nMagX:%.2f\nMagY:%.2f\nMagZ%.2f", IMU.getTemperature_C(), IMU.getMagX_uT(), IMU.getMagY_uT(), IMU.getMagZ_uT());
            lv_label_set_text(sensor_label, buf);
        }
    }

    lv_timer_handler();
    delay(1);
}


static void lvglHelper()
{
    static lv_disp_draw_buf_t disp_buf;  // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;       // contains callback functions

    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, LV_BUFFER_SIZE);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width();
    disp_drv.ver_res = tft.height();
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

}


static void lv_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
    tft.endWrite();
    lv_disp_flush_ready( drv );
}

