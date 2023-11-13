/**
 * @file      esp32c6_pins.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-23
 * @note      Demonstrates writing an ESP32-C6 example in the Arduino IDE
 *            How to install ESP32-C6 in Arduino IDE, please check out the quick start  https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html
 *
 */
#include <Arduino.h>

#define BOARD_WIFI_RX       15
#define BOARD_WIFI_TX       18
#define BOARD_WIFI_CTS      14
#define BOARD_WIFI_RTS      6
#define BOARD_WIFI_IRQ      7

const  uint8_t pins[] = {
    2,              //ESP_IO0
    3,              //ESP_IO1
    // RTS:6
    // CTS:14
    // RX:15
    // TX:18
    // USB DP:12,
    // USB DM:13
    4,              //ESP_IO2
    5,              //ESP_IO3
    // Reserve : 7
    8,              //ESP_IO4
    9,   //BOOT  Pin //ESP_IO5
    14,             //NO CONNECT
    15,             //NO CONNECT
    18,             //NO CONNECT
    19,             //ESP_IO6
    20,             //ESP_IO7
    21,             //ESP_IO8
    22,             //ESP_IO9
    23,             //ESP_IO10
    BOARD_WIFI_RX,
    BOARD_WIFI_TX,
    BOARD_WIFI_CTS,
    BOARD_WIFI_RTS,
    BOARD_WIFI_IRQ,
};

bool level = false;


void setup()
{
    for (auto i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        pinMode(pins[i], OUTPUT);
    }
}

void loop()
{
    for (auto i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        digitalWrite(pins[i], level);
    }
    level ^= 1;
    delay(10);
}
