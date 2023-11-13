/*******************************************************************************
 * Touch libraries:
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 *
 * Capacitive touchscreen libraries
 * TouchLib: https://github.com/mmMicky/TouchLib.git
 ******************************************************************************/
#include "utilities.h"

// Please fill below values from Arduino_GFX Example - TouchCalibration
int16_t touch_last_x = 0, touch_last_y = 0;

#include <Wire.h>
#include <TouchDrvCSTXXX.hpp>

TouchDrvCSTXXX touch;
int16_t x[5], y[5];
bool hasTouch = false;

void touch_init(int16_t w, int16_t h, uint8_t r)
{
    // Initialize capacitive touch
    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_IRQ);
    hasTouch = touch.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, CST226SE_SLAVE_ADDRESS);
}

bool touch_has_signal()
{
    return hasTouch;
}


bool touch_touched()
{
    uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());

    if (touched) {

        Serial.printf("X:%d Y:%d\n", x[0], y[0]);
        touch_last_x = x[0];
        touch_last_y = y[0];

        return true;
    }
    return false;
}

bool touch_released()
{
    return false;
}
