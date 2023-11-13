/**
 * @file      esp_gpio.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-11-09
 *
 */

#pragma once

#include <stdint.h>


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
bool testAT();
uint8_t esp_digital_read(uint8_t pin);
void esp_pin_mode(uint8_t pin, uint8_t mode);
void esp_digital_write(uint8_t pin, uint8_t val);


