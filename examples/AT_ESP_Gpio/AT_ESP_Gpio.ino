/**
 * @file      AT_ESP_Gpio.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-11-08
 * @note      The official AT firmware does not support GPIO control commands.
 * *          This is the modified AT firmware. Please see the source code here.
 * *          https://github.com/lewisxhe/esp-at
 * *          How compile see project https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/Compile_and_Develop/How_to_clone_project_and_compile_it.html
 * *          Other general AT commands
 * *          ESP32-C6: https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/Get_Started/index.html
 *
 * * */

#include <Arduino.h>
#include "utilities.h"

int8_t waitResponse(uint32_t timeout_ms, String &data, const char *r1 = "OK\r\n");
int8_t waitResponse(uint32_t timeout_ms = 2000UL, const char *r1 = "OK\r\n");

#define INPUT_OUTPUT    0x02

enum gpio_num {
    //GPIO :  0 ~ 12
    EIO0,
    EIO1,
    EIO2,
    EIO3,
    EIO4,
    EIO5,
    EIO6,
    EIO7,
    EIO8,
    EIO9,
    EIO10,
    ETX,
    ERX,
    EIO_MAX
};

//  @format AT+CGDRT: {GPIO},{0:IN, 1:OUTPUT,2:INPUT or OUTPUT,3:DISABLE}[,{0:Disable pullup,1:Enable pullup},{0:Disable pulldown,1:Enable pulldown}]
/// @brief AT+CGDRT={2,3,4,5,8,9,16,17,19,20,21,22,23},{0,1,2,3}[{0,1},{0,1}]
/// @param pin 2,3,4,5,8,9,16,17,19,20,21,22,23
/// @param mode  0:IN, 1:OUTPUT,2:INPUT or OUTPUT,3:DISABLE
void esp_pin_mode(uint8_t pin, uint8_t mode)
{
    char buffer[64] = {0};
    snprintf(buffer, 64, "AT+CGDRT=%d,%d\r\n", pin, mode);
    SerialAT.write(buffer);
    if (waitResponse() != 1) {
        Serial.println("Failed!");
    } else {
        Serial.println("Successed!");
    }
}

void esp_digital_write(uint8_t pin, uint8_t val)
{
    char buffer[64] = {0};
    snprintf(buffer, 64, "AT+CGSETV=%d,%d\r\n", pin, val);
    SerialAT.write(buffer);
    Serial.print ("send ..");
    if (waitResponse() != 1) {
        Serial.println("Failed!");
    } else {
        Serial.println("Successed!");
    }
}

uint8_t esp_digital_read(uint8_t pin)
{
    char buffer[64] = {0};
    snprintf(buffer, 64, "AT+CGGETV=%d\r\n", pin);
    SerialAT.write(buffer);
    String data;
    if (waitResponse(200, data) == 1) {
        return data.indexOf("1") > 0;
    }
    return 1;
}

bool testAT()
{
    SerialAT.print("AT\r\n");
    String input = SerialAT.readString();
    if (input.indexOf("OK") >= 0) {
        // Turn off echo function
        SerialAT.println("ATE0");
        SerialAT.readString();
        return true;
    }
    return false;
}


void setup()
{
    Serial.begin(115200);
    while (!Serial);

    // Ensure the UART pinout the WiFi Module is connected to is configured properly
    SerialAT.setRX(BOARD_WIFI_RX);
    SerialAT.setTX(BOARD_WIFI_TX);

    SerialAT.begin(115200);

    Serial.println("Test esp at");
    while (!testAT()) {
        delay(500);
    }
    Serial.println("Model is online !");

    //Set all gpio input output mode
    // 0x0:IN, 0x1:OUTPUT, 0x2:INPUT or OUTPUT, 0x3:DISABLE
    for (int i = 0; i < EIO_MAX; ++i) {
        esp_pin_mode(i, 0x02);
    }

}


int level = 0;

void loop()
{
    for (int i = 0; i < EIO_MAX; ++i) {
        //Toggle all gpio
        esp_digital_write(i, 1 - esp_digital_read(i));
    }
    delay(100);
}



int8_t waitResponse(uint32_t timeout_ms, String &data, const char *r1 )
{
    data.reserve(64);
    uint8_t  index       = 0;
    uint32_t startMillis = millis();
    do {
        while (SerialAT.available() > 0) {
            int8_t a = SerialAT.read();
            if (a <= 0) continue;  // Skip 0x00 bytes, just in case
            data += static_cast<char>(a);
            if (r1 && data.endsWith(r1)) {
                index = 1;
                goto finish;
            }
        }
    } while (millis() - startMillis < timeout_ms);
finish:
    if (!index) {
        data.trim();
        data = "";
    }
    return index;
}

int8_t waitResponse(uint32_t timeout_ms, const char *r1)
{
    String data;
    return waitResponse(timeout_ms, data, r1);
}