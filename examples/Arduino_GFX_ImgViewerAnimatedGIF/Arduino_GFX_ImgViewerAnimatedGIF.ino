/*******************************************************************************
 * Animated GIF Image Viewer
 * This is a simple Animated GIF image viewer example
 * Image Source: https://www.pexels.com/video/earth-rotating-video-856356/
 * cropped: x: 598 y: 178 width: 720 height: 720 resized: 240x240
 * optimized with ezgif.com
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload Animated GIF file
 *   FFat (ESP32):
 *     upload FFat (FatFS) data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   LittleFS (ESP32 / ESP8266 / Pico):
 *     upload LittleFS data with ESP8266 LittleFS Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *     ESP8266: https://github.com/earlephilhower/arduino-esp8266littlefs-plugin
 *     Pico: https://github.com/earlephilhower/arduino-pico-littlefs-plugin.git
 *   SPIFFS (ESP32):
 *     upload SPIFFS data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Most Arduino system built-in support SD file system.
 *     Wio Terminal require extra dependant Libraries:
 *     - Seeed_Arduino_FS: https://github.com/Seeed-Studio/Seeed_Arduino_FS.git
 *     - Seeed_Arduino_SFUD: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD.git
 ******************************************************************************/


#define GIF_FILENAME "/resize.gif"          // examples/Arduino_GFX_ImgViewerAnimatedGIF/data folder

#include <Arduino_GFX_Library.h>
#include "utilities.h"

#define GFX_BL BOARD_TFT_BL

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_RPiPicoSPI(BOARD_TFT_DC, BOARD_TFT_CS, BOARD_SPI_SCK, BOARD_SPI_MOSI, BOARD_SPI_MISO);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7796(bus,
                                      BOARD_TFT_RST,
                                      0 /* rotation */,
                                      true /* IPS */,
                                      222,
                                      480, 49, 0);

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(TARGET_RP2040)
#include <LittleFS.h>
#include <SD.h>
#elif defined(ESP32)
#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>
#elif defined(ESP8266)
#include <LittleFS.h>
#include <SD.h>
#else
#include <SD.h>
#endif

#include "GifClass.h"
static GifClass gifClass;

void setup()
{
    Serial.begin(115200);
    // Serial.setDebugOutput(true);
    // while (!Serial);
    Serial.println("Arduino_GFX Animated GIF Image Viewer example");

#ifdef GFX_EXTRA_PRE_INIT
    GFX_EXTRA_PRE_INIT();
#endif


    // Ensure the SPI pinout the SD card is connected to is configured properly
    SPI.setRX(BOARD_SPI_MISO);
    SPI.setTX(BOARD_SPI_MOSI);
    SPI.setSCK(BOARD_SPI_SCK);

    // see if the card is present and can be initialized:
    bool online = SD.begin(BOARD_SD_CS);

    // Init Display
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    gfx->fillScreen(WHITE);

#ifdef GFX_BL
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
#endif

    /* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI, 4000000UL))
#elif defined(TARGET_RP2040)
    // if (!LittleFS.begin())
    // if (!SD.begin(SS))
    if (!online)
#elif defined(ESP32)
    // if (!FFat.begin())
    if (!LittleFS.begin())
        // if (!SPIFFS.begin())
        // if (!SD.begin(SS))
#elif defined(ESP8266)
    if (!LittleFS.begin())
        // if (!SD.begin(SS))
#else
    if (!SD.begin())
#endif
    {
        Serial.println(F("ERROR: File System Mount Failed!"));
        gfx->println(F("ERROR: File System Mount Failed!"));
        exit(0);
    }
}

void loop()
{
    /* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
    File gifFile = SD.open(GIF_FILENAME, "r");
#elif defined(TARGET_RP2040)
    // File gifFile = LittleFS.open(GIF_FILENAME, "r");
    File gifFile = SD.open(GIF_FILENAME, "r");
#elif defined(ESP32)
    // File gifFile = FFat.open(GIF_FILENAME, "r");
    File gifFile = LittleFS.open(GIF_FILENAME, "r");
    // File gifFile = SPIFFS.open(GIF_FILENAME, "r");
    // File gifFile = SD.open(GIF_FILENAME, "r");
#elif defined(ESP8266)
    File gifFile = LittleFS.open(GIF_FILENAME, "r");
    // File gifFile = SD.open(GIF_FILENAME, "r");
#else
    File gifFile = SD.open(GIF_FILENAME, FILE_READ);
#endif
    if (!gifFile || gifFile.isDirectory()) {
        Serial.println(F("ERROR: open gifFile Failed!"));
        gfx->println(F("ERROR: open gifFile Failed!"));
    } else {
        // read GIF file header
        gd_GIF *gif = gifClass.gd_open_gif(&gifFile);
        if (!gif) {
            Serial.println(F("gd_open_gif() failed!"));
        } else {
            uint8_t *buf = (uint8_t *)malloc(gif->width * gif->height);
            if (!buf) {
                Serial.println(F("buf malloc failed!"));
            } else {
                int16_t x = (gfx->width() - gif->width) / 2;
                int16_t y = (gfx->height() - gif->height) / 2;

                Serial.println(F("GIF video start"));
                int32_t t_fstart, t_delay = 0, t_real_delay, delay_until;
                int32_t res = 1;
                int32_t duration = 0, remain = 0;
                while (res > 0) {
                    t_fstart = millis();
                    t_delay = gif->gce.delay * 10;
                    res = gifClass.gd_get_frame(gif, buf);
                    if (res < 0) {
                        Serial.println(F("ERROR: gd_get_frame() failed!"));
                        break;
                    } else if (res > 0) {
                        gfx->drawIndexedBitmap(x, y, buf, gif->palette->colors, gif->width, gif->height);

                        t_real_delay = t_delay - (millis() - t_fstart);
                        duration += t_delay;
                        remain += t_real_delay;
                        delay_until = millis() + t_real_delay;
                        while (millis() < delay_until) {
                            delay(1);
                        }
                    }
                }
                Serial.println(F("GIF video end"));
                Serial.print(F("duration: "));
                Serial.print(duration);
                Serial.print(F(", remain: "));
                Serial.print(remain);
                Serial.print(F(" ("));
                Serial.print(100.0 * remain / duration);
                Serial.println(F("%)"));

                gifClass.gd_close_gif(gif);
                free(buf);
            }
        }
    }
}
