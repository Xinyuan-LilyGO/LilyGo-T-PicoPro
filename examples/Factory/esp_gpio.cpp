/**
 * @file      AT_ESP_Gpio.cpp
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

int8_t waitResponse(uint32_t timeout_ms, String &data, const char *r1 )
{
    data.reserve(64);
    uint8_t  index       = 0;
    uint32_t startMillis = millis();
    do {
        while (SerialAT.available() > 0) {
            int8_t a = SerialAT.read();
            Serial.write(a);
            if (a <= 0) continue;
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