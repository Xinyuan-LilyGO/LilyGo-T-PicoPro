/****************************************************************************************************************************
  basic_pwm.ino
  For RP2040 boards
  Written by Dr. Benjamin Bird

  A basic example to get you up and running.

  Library by Khoi Hoang https://github.com/khoih-prog/RP2040_PWM
  Licensed under MIT license

  The RP2040 PWM block has 8 identical slices. Each slice can drive two PWM output signals, or measure the frequency
  or duty cycle of an input signal. This gives a total of up to 16 controllable PWM outputs. All 30 GPIO pins can be driven
  by the PWM block
*****************************************************************************************************************************/
// More PWM examples see  https://github.com/khoih-prog/RP2040_PWM

#define _PWM_LOGLEVEL_        3
#include <Arduino.h>
#include "RP2040_PWM.h"

//creates pwm instance
RP2040_PWM *PWM_Instance;

float frequency = 1000;
float dutyCycle;

// T-Pico-Pro Can use pin list : 19,22,23,24,25,26,27,28
#define pinToUse      25

void setup()
{
    //assigns pin 25 (built in LED), with frequency of 1 KHz and a duty cycle of 0%
    PWM_Instance = new RP2040_PWM(pinToUse, frequency, 0);
}

void loop()
{
    delay(1000);
    dutyCycle = 50;

    PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);

    delay(1000);
    dutyCycle = 10;

    PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);
}
