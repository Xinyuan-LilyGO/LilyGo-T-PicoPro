/**
 * @file      TFT_eSPI.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-10-17
 *
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "image.h"
#include "utilities.h"

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

TFT_eSPI tft = TFT_eSPI();

// 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum brightness
void setBrightness(uint8_t value)
{
    static uint8_t level = 0;
    static uint8_t steps = 16;
    if (value == 0) {
        digitalWrite(BOARD_TFT_BL, LOW);
        delay(3);
        level = 0;
        return;
    }
    if (level == 0) {
        digitalWrite(BOARD_TFT_BL, HIGH);
        level = steps;
        delayMicroseconds(30);
    }
    int from = steps - level;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BOARD_TFT_BL, LOW);
        digitalWrite(BOARD_TFT_BL, HIGH);
    }
    level = value;
}


void setup()
{
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(1);
    tft.setSwapBytes(true);

    tft.fillScreen(TFT_RED);
    delay(300);
    tft.fillScreen(TFT_GREEN);
    delay(300);
    tft.fillScreen(TFT_BLUE);
    delay(300);


}

void loop()
{
    Serial.println(millis());

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Test", 0, 0, 4);
    tft.drawString("1234567890", 0, 40, 2);
    tft.drawString("1234567890", 0, 80, 4);
    delay(5000);

    tft.pushImage(0, 0, tft.width(), tft.height(), (uint16_t *)gImage_image);

    // adjus tft brightness , 16 level
    for (int i = 0; i < 16; ++i) {
        setBrightness(i);
        delay(200);
    }

    delay(5000);
}
