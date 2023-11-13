/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-11
 *
 */
#pragma once
// https://www.lilygo.cc/products/t-pico-pro
// T-Pico-Pro pinmap

// LTR553 , TOUCH , SY6970 share I2C Bus
#define BOARD_I2C_SDA       0
#define BOARD_I2C_SCL       1

// Not use
#define QWIIC_I2C_SDA       2
#define QWIIC_I2C_SCL       3

#define QWIIC_UART_RX       21
#define QWIIC_UART_TX       30


// WIFI Module
#define BOARD_WIFI_RX       13
#define BOARD_WIFI_TX       12
#define BOARD_WIFI_CTS      14
#define BOARD_WIFI_RTS      15
#define BOARD_WIFI_EN       25
#define BOARD_WIFI_IRQ      22


// SD , TFT share SPI Bus
#define BOARD_SPI_MISO      4
#define BOARD_SPI_MOSI      7
#define BOARD_SPI_SCK       6
#define BOARD_TFT_CS        8
#define BOARD_TFT_DC        9
#define BOARD_TFT_RST       16
#define BOARD_TFT_BL        10
#define BOARD_SD_CS         5

#define BOARD_SENSOR_IRQ    19
#define BOARD_TOUCH_RST     17
#define BOARD_TOUCH_IRQ     18
#define BOARD_TFT_WIDTH     222
#define BOARD_TFT_HEIHT     480

// BUTTON Pinmap
#define BOARD_USER_BUTTON   {23 /*btn1*/,24/*btn2*/}
#define BOARD_USER_BTN_NUM  2


