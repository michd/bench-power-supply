// IMPORTANT NOTE:
// I refactored my initial sketchy code, but this was only after
// I'd broken down the breadboarded test setup (the beast had 54 wires!)
// What I'm saying is that I can't promise this works.
// This was on an Arduino Uno r3, with an atmega328.

// You can find the LedControl library here: http://playground.arduino.cc/Main/LedControl
// (Download is somewhere near the bottom of the page)
#include "LedControl.h"

// Pins for the MAX7219 serial interface
#define P_LED_DISPLAY_DATA_IN 12
#define P_LED_DISPLAY_CLK 11
#define P_LED_DISPLAY_LOAD 10
// LED intensity, Range of 0x0-0xF, where 0xF is brightest
#define LED_DISPLAY_INTENSITY 0x8
// Interval in ms
#define LED_DISPLAY_UPDATE_INTERVAL 250

// ADC input pins
#define P_MEASURE_VOLTAGE A0
#define P_MEASURE_CURRENT A1

// LedControl instance for the two 4-digit 7-segment displays
LedControl lc = LedControl(
      P_LED_DISPLAY_DATA_IN,
      P_LED_DISPLAY_CLK,
      P_LED_DISPLAY_LOAD,
      1
    );


void setup() {  
  pinMode(P_MEASURE_VOLTAGE, INPUT);
  pinMode(P_MEASURE_CURRENT, INPUT);
  
  // Setup display
  lc.setIntensity(0, LED_DISPLAY_INTENSITY); //0-15
  lc.clearDisplay(0);
}

void loop() {
  writeNumber(readVoltage(), 0);
  writeNumber(readCurrent(), 1);
  
  // Yeah, this isn't going to stay.
  delay(LED_DISPLAY_UPDATE_INTERVAL);
}

/**
 * Reads the present output voltage from the ADC and
 * converts it to the actual base-10 value in Volts.
 *
 * Voltage is measured through a voltage divider,
 * bringing it down from 39V to a scale of 0-5V,
 * suitable for the analog to digital converter.
 * 
 * It is converted from the chip's 0-1023 scale to an
 * actual voltage level, and then multiplied to be on
 * the correct scale.
 *
 * @return {float}
 * @todo Reform formula to use a well-named constant for calibration
 */
float readVoltage() {
  int inputValue = analogRead(P_MEASURE_VOLTAGE); //0-1023
  return ((float) (inputValue)) / 26.23;
}

/**
 * Reads the present output current from the ADC and
 * converts it to the actual base-10 value in Amperes.
 *
 * Current is measured by placing a ~1ohm resistor in series
 * with the load, and then measuring the voltage across this
 * 1 ohm resistor.
 * 
 * Since we're dealing with well under 5 amps, this doesn't
 * require a voltage divider.
 *
 * @return {float}
 * @todo Reform formula to use a well-named constant for calibration
 */
float readCurrent() {
  int inputValue = analogRead(P_MEASURE_CURRENT); // 0-1023
  return (((float) (inputValue)) / 265.0);
}

/**
 * Writes a number on a 4-digit display. Range: 0.00-99.99
 *
 * @param {float} number
 * @param {int} displayIndex - signifies a 4 digit display
 * @todo allow setting of number of decimal places
 * @todo use an array of digits instead of badly named vars
 */
void writeNumber(float number, int displayIndex) {
  int tens = 0;
  int ones = 0;
  int tenths = 0;
  int hundredths = 0;
  
  // Separate float into digits (this needs work)
  if (number >= 10) {
    tens = (int) (number / 10);
  }
  
  number -= (tens * 10);
  
  ones = (int) number;
  
  number -= ones;

  number = 100 * number;
  if (number >= 10) {
    tenths = (int) (number / 10);
  }
  number -= (tenths * 10);
  hundredths = ((int) (number)) % 10;
  
  // Turn of display before updating data on it
  lc.shutdown(0, true);
  
  // Don't display a padding 0
  if (tens > 0) {
    lc.setDigit(0, (displayIndex * 4) + 3, tens, false);
  } else {
    lc.setChar(0, (displayIndex * 4) + 3, (byte)127, false);
  }
  lc.setDigit(0, (displayIndex * 4) + 2, ones, true);
  lc.setDigit(0, (displayIndex * 4) + 1, tenths, false);
  lc.setDigit(0, (displayIndex * 4) + 0, hundredths, false);
 
  // Turn the display back on 
  lc.shutdown(0, false);
}
