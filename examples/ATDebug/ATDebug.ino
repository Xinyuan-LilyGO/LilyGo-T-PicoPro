/**
 * @file      ATDebug.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-10-11
 *
 */
#include <Arduino.h>
#include "utilities.h"

// AT Command Set:
// ESP32-C6: https://docs.espressif.com/projects/esp-at/en/latest/esp32c6/Get_Started/index.html

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    // Ensure the UART pinout the WiFi Module is connected to is configured properly
    SerialAT.setRX(BOARD_WIFI_RX);
    SerialAT.setTX(BOARD_WIFI_TX);

    SerialAT.begin(115200);
}



void loop()
{
    while (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    while (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    delay(5);
}

