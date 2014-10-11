#include "LedControl.h"
#include "ClickEncoder.h"
#include "TimerOne.h"

// # Configuration
#define ADC_REFERENCE_VOLTAGE 5.0
#define ADC_PRECISION 1024

// Display
#define DISP_INTENSITY 0x8
#define DISP_DIGITS 4

// Voltage control
#define REGULATOR_VOLTAGE_OFFSET 2.25
#define COARSE_VOLTAGE_RANGE 30
#define FINE_VOLTAGE_RANGE 5
// Adds up to 35V
#define VOLTAGE_STEP 0.05
#define CURRENT_STEP 0.005
#define VOLTAGE_DECIMAL_POINTS 2
#define CURRENT_DECIMAL_POINTS 3

#define VOLTAGE_DISP_OFFSET 0
#define CURRENT_DISP_OFFSET DISP_DIGITS

// Fine-tune these for measuring calibration
#define CURRENT_MEASURING_RESISTANCE 0.788
#define VOLTAGE_MEASURING_MULTIPLIER 9.4

// # Pins
// I/O board
#define O_DISP_DATA_IN 12
#define O_DISP_CLOCK   11
#define O_DISP_LOAD    10
#define I_ROTA_VOLTAGE A2
#define I_ROTB_VOLTAGE A3
#define I_BTN_VOLTAGE   8
#define I_ROTA_CURRENT A4
#define I_ROTB_CURRENT A5
#define I_BTN_CURRENT   9

// Voltage / current measuring
#define AI_MEASURE_VOLTAGE A0
#define AI_MEASURE_CURRENT A1

// Voltage control PWM
#define PWMO_VOLTAGE_COARSE 5
#define PWMO_VOLTAGE_FINE   6

// Set up display controller
LedControl disp = LedControl(
  O_DISP_DATA_IN,
  O_DISP_CLOCK,
  O_DISP_LOAD,
  1
);

ClickEncoder *voltageKnob;
ClickEncoder *currentKnob;

float targetVoltage; // 1.50-35.00
float targetCurrentLimit; //0.000-2.500

//==================================================================

void readEncoders() {
  voltageKnob->service();
  currentKnob->service();
  // NOTE: Since this is triggered 1000 times per second,
  // it might be a good idea to read current and voltage
  // from here, if possible.
  // If I were to do this though, it'd be good to not convert
  // the int readout to a float, to save cycles. When
  // changing target voltage and target current, their values
  // should also be stored in the relevant int form, for more
  // efficient comparison with read values.
  // Also uh, I guess readEncoders wouldn't be a very accurate name.
}

void setup() {
  // Initialize rotary encoders
  voltageKnob = new ClickEncoder(
    I_ROTA_VOLTAGE,
    I_ROTB_VOLTAGE,
    I_BTN_VOLTAGE
  );
  
  currentKnob = new ClickEncoder(
    I_ROTA_CURRENT,
    I_ROTB_CURRENT,
    I_BTN_CURRENT
  );
  
  // Configure display
  disp.setIntensity(0, DISP_INTENSITY);
  disp.clearDisplay(0);
  
  // Set up timer for reading rotary encoders
  Timer1.initialize(1000);
  Timer1.attachInterrupt(readEncoders);
  
  //Pin modes for measuring voltage and current
  pinMode(AI_MEASURE_VOLTAGE, INPUT);
  pinMode(AI_MEASURE_CURRENT, INPUT);
  
  // Pin modes for PWM outputs for controller voltage and current
  pinMode(PWMO_VOLTAGE_COARSE, OUTPUT);
  pinMode(PWMO_VOLTAGE_FINE, OUTPUT); 
  Serial.begin(9600);
}

void loop() {
  writeVoltage(10.0);
  displayVoltage(readVoltage());
  displayCurrent(readCurrent());
   
  delay(500);
}

/**
 * Reads voltage from analog input and translates it to the
 * actual voltage value.
 *
 * Also depends on readCurrent, as the voltage drop over the
 * current measuring resistors needs to be subtracted from
 * the voltage read.
 * 
 * Value from ADC ranges 0-1023, which maps to 0-39V at 
 * 0.038V precision.
 * 
 * Value is rounded to the nearest 0.05V.
 *
 * @todo test.
 */
float readVoltage() {
  int inputValue = analogRead(AI_MEASURE_VOLTAGE);
  
  float resultingVoltage = (
      (
        (float) inputValue /
        (ADC_PRECISION / ADC_REFERENCE_VOLTAGE)
      ) *
      VOLTAGE_MEASURING_MULTIPLIER // That is, voltage divider ratio
    ) -
    readCurrent() * CURRENT_MEASURING_RESISTANCE;
    
  return resultingVoltage;
}

/**
 * Reads voltage from analog input and translates it to the
 * actual current value.
 *
 * Value from ADC technically ranges 0-1023, which maps to 0-5A at
 * 0.005 precision, which we'll keep.
 *
 * Actual values should not go far beyond 2.5A. Software limited.
 *
 * @todo test.
 */
float readCurrent() {
  int inputValue = analogRead(AI_MEASURE_CURRENT);
  
  float resultingCurrent = (           
      (float) inputValue / 
      (ADC_PRECISION / ADC_REFERENCE_VOLTAGE) 
    ) / 
    CURRENT_MEASURING_RESISTANCE;
    
  return resultingCurrent;
}

/**
 * Sets the control voltage, connected to voltage adjust pins on
 * regulators.
 *
 * Takes into account measured current and target current limit first,
 * and target voltage second.
 *
 * @todo implement. 
 */
void writeVoltage(float voltage)
{
  voltage -= (float) REGULATOR_VOLTAGE_OFFSET;
  int coarse = (int)voltage;
  float fine;
  if (coarse > COARSE_VOLTAGE_RANGE)
  {
    coarse -= (coarse - COARSE_VOLTAGE_RANGE);
  }
  
  fine = voltage - (float) coarse;
  writeCoarseVoltage(coarse);
  writeFineVoltage(fine);
}

void writeCoarseVoltage(int cVoltage)
{
  //COARSE_VOLTAGE_RANGE = output 255
  int cAnalog = (int) ((255.0 / (float) COARSE_VOLTAGE_RANGE) * cVoltage);
  analogWrite(PWMO_VOLTAGE_COARSE, cAnalog);  
}

void writeFineVoltage(float fVoltage)
{
  int fAnalog = (int) ((255.0 / (float) FINE_VOLTAGE_RANGE) * fVoltage);
  analogWrite(PWMO_VOLTAGE_FINE, fAnalog);
}
// ########################################################
// ## Format and display voltage and current on displays ##
// ########################################################

/**
 * Shows a given float voltage value on the relevant display.
 * 
 * Should display it in format [X]X.XX
 *
 * Can probably use a common helper method for actual displaying.
 * 
 * @note Should employ a rounding function to round to nearest 0.05V
 * @note This implementation will need extensive testing.
 * @todo Implement.
 */
void displayVoltage(float voltage)
{
  float roundedVoltage = roundToNearest(voltage, VOLTAGE_STEP);
  signed char digits[DISP_DIGITS];
  prepareForDisplay(roundedVoltage, VOLTAGE_DECIMAL_POINTS, digits);
  writeDisplay(VOLTAGE_DISP_OFFSET, VOLTAGE_DECIMAL_POINTS, digits);
}

/**
 * Shows a given float current value on the relevant display.
 *
 * Should display it in format X.XXX
 *
 * Can probably use a common helper method for actual displaying.
 *
 * @note Should employ a rounding function to round to nearest 0.005A
 * @note This implementation will need extensive testing
 * @todo Implement.
 */
void displayCurrent(float current) 
{
  float roundedCurrent = roundToNearest(current, CURRENT_STEP);
  signed char digits[DISP_DIGITS];
  prepareForDisplay(roundedCurrent, CURRENT_DECIMAL_POINTS, digits);
  writeDisplay(CURRENT_DISP_OFFSET, CURRENT_DECIMAL_POINTS, digits);
}

void writeDisplay(char displayOffset, char decimalPoints, signed char* digits)
{
  char i;
  disp.shutdown(0, true);
  
  for(i = 0; i < DISP_DIGITS; i += 1)
  {
    disp.setDigit(0, i + displayOffset, digits[i], i == DISP_DIGITS - (decimalPoints + 1));
  }
  
  disp.shutdown(0, false);
}

// Below stuff has been tested with gcc on a PC

/**
 * Rounds a float value to the nearest <rounding>
 * 
 * Used to round voltage and current measurements to 0.05V and 0.005A
 *
 * @note Only works somewhat reliably with roundings ending in 5
 * @todo Investigate using some stdlib stuff to make this neater,
 *       yet still not riddled with floating point errors.
 */
float roundToNearest(float input, float rounding)
{
  int preMultiplier = 1;
  int intVersion;

  if (rounding < 1)  preMultiplier = 10;
  if (rounding < 0.1) preMultiplier = 100;
  if (rounding < 0.01) preMultiplier = 1000;

  intVersion = (int)(input * preMultiplier);

  int intDivided = (int)((float)intVersion / (rounding * preMultiplier));
  int remainderInt = (((float)intVersion / (rounding * preMultiplier)) - intDivided) * 10;

  int roundingLeftOver = (int)((float)remainderInt / (rounding * preMultiplier));
  float roundingRemainder = ((float)remainderInt / (rounding * preMultiplier)) - roundingLeftOver;

  if (roundingRemainder >= ((rounding * preMultiplier) / 2))
  {
    roundingLeftOver += 1;
  }

  int result = (intDivided * (rounding * preMultiplier)) + (roundingLeftOver * (rounding * preMultiplier)); 
  return (float)result / preMultiplier;
}

/**
 * Retrieves digits from a float value, in an array of chars
 * 
 * Used for preparing to display float values on 7-segment displays
 */
void prepareForDisplay(float input, char decimalPoints, signed char* outDigits)
{
  int intInput = (int)(((int)(input * power(10, decimalPoints))) % power(10, DISP_DIGITS));
  char i;
  signed char digit;
  for (i = DISP_DIGITS - 1; i >= 0; i--)
  {
    digit = (signed char)(((intInput % power(10, i + 1)) - (intInput % power(10, i))) / power(10, i));
    // Don't display a digit at [X]x.xx if [X] is 0
    if (digit == 0 && i == (decimalPoints + 1))
    {
      digit = -1;
    }
    outDigits[DISP_DIGITS - i - 1] = digit;
  }
}

/**
 * Integer implementation of pow
 */
int power(int base, int exp)
{
  int i;
  int result;

  if (exp == 0)
  {
    return 1;
  }

  result = base;

  for (i = 1; i < exp; i++)
  {
    result = result * base;
  }

  return result;
}
