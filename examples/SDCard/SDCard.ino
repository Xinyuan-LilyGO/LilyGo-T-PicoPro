/**
 * @file      SDCard.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-11-07
 *
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
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

File root;

void printDirectory(File dir, int numTabs)
{
    while (true) {

        File entry =  dir.openNextFile();
        if (! entry) {
            // no more files
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        Serial.print(entry.name());
        if (entry.isDirectory()) {
            tft.println("/");

            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.print(entry.size(), DEC);
            time_t cr = entry.getCreationTime();
            time_t lw = entry.getLastWrite();
            struct tm *tmstruct = localtime(&cr);
            Serial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
            tft.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
            tmstruct = localtime(&lw);
            Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
            tft.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        entry.close();
    }
}


void setup()
{
    Serial.begin(115200);

    // Ensure the SPI pinout the SD card is connected to is configured properly
    SPI.setRX(BOARD_SPI_MISO);
    SPI.setTX(BOARD_SPI_MOSI);
    SPI.setSCK(BOARD_SPI_SCK);

    // see if the card is present and can be initialized:
    bool online = SD.begin(BOARD_SD_CS);


    tft.begin();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    if (!online) {
        tft.setTextColor(TFT_RED);
        Serial.println("Card failed, or not present");
        tft.println("Card failed, or not present");
        while (1) {
            delay(1);
        }
    } else {
        tft.setTextColor(TFT_GREEN);
        Serial.print("SD initialization done.");
        Serial.print(SD.size64() / 1024 / 1024.0);
        Serial.println("MB");
        tft.print("SD initialization done.");
        tft.print(SD.size64() / 1024 / 1024.0);
        tft.println("MB");
    }

    delay(2000);

    root = SD.open("/");

    printDirectory(root, 0);

    root.close();

    delay(2000);

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.

    root = SD.open("test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (root) {
        Serial.print("Writing to test.txt...");
        tft.print("Writing to test.txt...");
        root.println("testing 1, 2, 3.");
        // close the file:
        root.close();
        Serial.println("done.");
        tft.println("done.");
    } else {
        // if the file didn't open, print an error:
        Serial.println("error opening test.txt");
        tft.println("error opening test.txt");
    }

    // re-open the file for reading:
    root = SD.open("test.txt");
    if (root) {
        Serial.println("test.txt:");
        tft.println("test.txt:");

        // read from the file until there's nothing else in it:
        while (root.available()) {
            int c = root.read();
            Serial.write(c);
            tft.write(c);
        }
        // close the file:
        root.close();
    } else {
        // if the file didn't open, print an error:
        Serial.println("error opening test.txt");
        tft.println("error opening test.txt");
    }

}

void loop()
{
}
