#include "LedControl.h"
#include "ClickEncoder.h"
#include "TimerOne.h"

// # Configuration
// uC properties
#define ADC_REFERENCE_VOLTAGE 5.0
#define ADC_PRECISION 1024
#define PWM_PRECISION 255.0

// Display properties
// Display brightness, 0x0-0xF
#define DISP_INTENSITY 0x8
// Number of digits available per display
#define DISP_DIGITS 4

// Voltage control
// According to the datasheet this offset should be 1.30V max,
// but in practice I needed some more
#define REGULATOR_VOLTAGE_OFFSET 2.25

//Output voltage is controlled with 2 PWM outputs that get summed,
//Because of PWM precision limitations
#define COARSE_VOLTAGE_RANGE 30
#define FINE_VOLTAGE_RANGE 5

// Voltage precision of the PSU in Volts,
// used for rounding measured voltage and stepping through configured voltage
#define VOLTAGE_STEP 0.05
// Used for display point
#define VOLTAGE_DECIMAL_POINTS 2
// Some software limits here, 2.5 because of REGULATOR_VOLTAGE_OFFSET
// and rounding
#define VOLTAGE_TARGET_MINIMUM 2.5
#define VOLTAGE_TARGET_MAXIMUM 35.0
// Voltage that gets set on system startup
#define VOLTAGE_TARGET_INIT VOLTAGE_TARGET_MINIMUM

// Current precision of the PSU in Amps,
// used for rounding measurements and stepping through current limit value
#define CURRENT_STEP 0.005
// Used for display point
#define CURRENT_DECIMAL_POINTS 3
// Some software limits here
#define CURRENT_TARGET_MINIMUM 0.0
#define CURRENT_TARGET_MAXIMUM 2.5
// Current limit that gets set on system startup
#define CURRENT_TARGET_INIT 0.5

// Circumvents advancing two units per notch when turning rotary encoder:
#define UNITS_PER_ENCODER_NOTCH 2 

// MAX7219CNG sees our two displays as a single 8-digit one, so:
// Digit index where voltage display starts
#define VOLTAGE_DISP_OFFSET 0
// Digit index where current display starts
#define CURRENT_DISP_OFFSET DISP_DIGITS

// Fine-tune these for measuring calibration
// Resistance in series with load through which current is measured
// When measured with a multimeter, this would've been 1.015, yet
// actual calibration came up with this value.
#define CURRENT_MEASURING_RESISTANCE 0.788
// Measured voltage is voltage-divided to stay within the ADC_REFERENCE VOLTAGE
#define VOLTAGE_MEASURING_MULTIPLIER 9.4

// For the main loop()
#define LOOP_CYCLE_DELAY 200

// When in setting mode on a display, revert back to measurement
// after this many ms
#define SETTING_DISP_TIMEOUT 3000

// These cycles are used for blinking the display out when in setting mode
#define DISPLAY_CYCLES 5

// # Pins
// Legend: [A|PWM]<O|I>_<name>
// A = analog (for inputs)
// PWM = pulse width modulation output
// O = output
// I = input
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

enum displayMode {
  measuring,
  setting
};

LedControl disp = LedControl(
  O_DISP_DATA_IN,
  O_DISP_CLOCK,
  O_DISP_LOAD,
  1  
);

ClickEncoder *voltageKnob;
ClickEncoder *currentKnob;

// Voltage meant to be output (V)
float targetVoltage;
// Current to limit to. output voltage can and will be lowered
// to enforce this limit. (A)
float targetCurrentLimit;

// Latest output voltage measurement (V)
float voltageMeasurement;

// Latest output current measurement (A)
float currentMeasurement;


displayMode voltageDisplayMode = measuring;
displayMode currentDisplayMode = measuring;

// Remaining time before display goes back to measurement mode when in setting mode
int voltageDisplayModeTimeout = 0;
int currentDisplayModeTimeout = 0;

//Used for blinking display when in setting mode
char displayCycleStep = 0;


//==================================================================

void readInputs() {
  voltageKnob->service();
  currentKnob->service();
  
  currentMeasurement = readCurrent();
  voltageMeasurement = readVoltage();
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
  
  targetVoltage = VOLTAGE_TARGET_INIT;
  targetCurrentLimit = CURRENT_TARGET_INIT;
  
  // Set up timer for reading rotary encoders
  // and voltage/current measurements
  Timer1.initialize(1000);
  Timer1.attachInterrupt(readInputs);
  
  //Pin modes for measuring voltage and current
  pinMode(AI_MEASURE_VOLTAGE, INPUT);
  pinMode(AI_MEASURE_CURRENT, INPUT);
  
  // Pin modes for PWM outputs for controller voltage and current
  pinMode(PWMO_VOLTAGE_COARSE, OUTPUT);
  pinMode(PWMO_VOLTAGE_FINE, OUTPUT); 

  // Makes displays work for some reason.
  // TODO: figure out why and elimate need for Serial in here
  Serial.begin(9600);
}

void loop() {
  int voltageKnobDiff = (int)(voltageKnob->getValue() / UNITS_PER_ENCODER_NOTCH);
  int currentKnobDiff = (int)(currentKnob->getValue() / UNITS_PER_ENCODER_NOTCH);
  
  // Voltage knob moved
  if (voltageKnobDiff != 0) {    
    voltageDisplayMode = setting; 
    adjustTargetVoltage(voltageKnobDiff);
    voltageDisplayModeTimeout = SETTING_DISP_TIMEOUT;
  }
  
  // Voltage knob button clicked
  if (voltageKnob->getButton() == ClickEncoder::Clicked) {
    if (voltageDisplayMode == setting) {
      voltageDisplayMode = measuring;
      voltageDisplayModeTimeout = 0;
    } else {
      voltageDisplayMode = setting;
      voltageDisplayModeTimeout = SETTING_DISP_TIMEOUT;
    }
  }
  
  // Current knob moved
  if (currentKnobDiff != 0) {
    currentDisplayMode = setting;
    adjustTargetCurrentLimit(currentKnobDiff);
    currentDisplayModeTimeout = SETTING_DISP_TIMEOUT;
  }
  
  // Current knob button clicked
  if (currentKnob->getButton() == ClickEncoder::Clicked) {
    if (currentDisplayMode == setting) {
      currentDisplayMode = measuring;
      currentDisplayModeTimeout = 0;
    } else {
      currentDisplayMode = setting;
      currentDisplayModeTimeout = SETTING_DISP_TIMEOUT;
    }
  }
  
  updateDisplays();

  delay(LOOP_CYCLE_DELAY);
}

/**
 * Adjust the target voltage by a given number of steps
 *
 * Constrains the target voltage to a minimum and maximum
 */
void adjustTargetVoltage(int steps) {
  targetVoltage += (steps * VOLTAGE_STEP);
  if (targetVoltage < VOLTAGE_TARGET_MINIMUM) targetVoltage = VOLTAGE_TARGET_MINIMUM;
  if (targetVoltage > VOLTAGE_TARGET_MAXIMUM) targetVoltage = VOLTAGE_TARGET_MAXIMUM;
}

/**
 * Adjust the target current limit by a given number of steps
 *
 * Constrains the target current to a minimum and maximum
 */
void adjustTargetCurrentLimit(int steps) {
  targetCurrentLimit += (steps * CURRENT_STEP);
  if (targetCurrentLimit < CURRENT_TARGET_MINIMUM) targetCurrentLimit = CURRENT_TARGET_MINIMUM;
  if (targetCurrentLimit > CURRENT_TARGET_MAXIMUM) targetCurrentLimit = CURRENT_TARGET_MAXIMUM;
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
 */
float readVoltage() {
  int inputValue = analogRead(AI_MEASURE_VOLTAGE);
  
  return (
      (
        (float) inputValue /
        (ADC_PRECISION / ADC_REFERENCE_VOLTAGE)
      ) *
      VOLTAGE_MEASURING_MULTIPLIER // That is, voltage divider ratio
    ) -
    currentMeasurement * CURRENT_MEASURING_RESISTANCE;
}

/**
 * Reads voltage from analog input and translates it to the
 * actual current value.
 *
 * Value from ADC technically ranges 0-1023, which maps to 0-5A at
 * 0.005 precision, which we'll keep.
 *
 * Actual values should not go far beyond 2.5A. Software limited.
 */
float readCurrent() {
  int inputValue = analogRead(AI_MEASURE_CURRENT);
  
  return (           
      (float) inputValue / 
      (ADC_PRECISION / ADC_REFERENCE_VOLTAGE) 
    ) / 
    CURRENT_MEASURING_RESISTANCE;
}

/**
 * Intermediate function for writeVoltage, adjusts voltage for current limiting
 */
void adjustOutput() {
  float outputVoltage = targetVoltage;
  float currentMeasuringVoltageDrop = (float) (currentMeasurement * CURRENT_MEASURING_RESISTANCE);
  float estimatedLoadResistance = voltageMeasurement / currentMeasurement;
  
  // Adjust for current limit 
  if (targetCurrentLimit - currentMeasurement < -CURRENT_STEP) {
      outputVoltage = (targetCurrentLimit * estimatedLoadResistance) + 
                      (targetCurrentLimit * CURRENT_MEASURING_RESISTANCE);
  } else {
    // Adjust for current measuring resistor voltage drop
    // This might need tweaking depending on stability of the analog input measurements
    if (targetVoltage - voltageMeasurement > VOLTAGE_STEP || currentMeasuringVoltageDrop > VOLTAGE_STEP) {
      outputVoltage += currentMeasuringVoltageDrop;
    }
  }
  writeVoltage(outputVoltage);
}
  

/**
 * Sets the control voltage, connected to voltage adjust pins on
 * regulators.
 */
void writeVoltage(float voltage) {
  voltage -= (float) REGULATOR_VOLTAGE_OFFSET;
   
  int coarse = (int)voltage;
  float fine;
  if (coarse > COARSE_VOLTAGE_RANGE) coarse -= (coarse - COARSE_VOLTAGE_RANGE);
  
  fine = voltage - (float) coarse;
  writeCoarseVoltage(coarse);
  writeFineVoltage(fine);
}

void writeCoarseVoltage(int cVoltage) {
  int cAnalog = (int) ((PWM_PRECISION / (float) COARSE_VOLTAGE_RANGE) * cVoltage);
  analogWrite(PWMO_VOLTAGE_COARSE, cAnalog);  
}

void writeFineVoltage(float fVoltage) {
  int fAnalog = (int) ((PWM_PRECISION / (float) FINE_VOLTAGE_RANGE) * fVoltage);
  analogWrite(PWMO_VOLTAGE_FINE, fAnalog);
}

// ########################################################
// ## Format and display voltage and current on displays ##
// ########################################################

/**
 * Updates voltage and current displays with their relevant values.
 *
 * Takes into account whether the display is in measure or setting mode
 * When in setting mode: 
 *   - Blinks the display out every 5 times this is called
 *   - Reverts to measurement mode after a timeout
 */
void updateDisplays() {
  // Voltage display mode control
  if (voltageDisplayMode == setting) {
    voltageDisplayModeTimeout -= LOOP_CYCLE_DELAY;
    if (voltageDisplayModeTimeout <= 0) {
      voltageDisplayModeTimeout = 0;
      voltageDisplayMode = measuring;
    }
  }
  
  // Current display mode control
  if (currentDisplayMode == setting) {
    currentDisplayModeTimeout -= LOOP_CYCLE_DELAY;
    if (currentDisplayModeTimeout <= 0) {
      currentDisplayModeTimeout = 0;
      currentDisplayMode = measuring;
    }
  }
  
  // Voltage display
  if (voltageDisplayMode == setting) {
    if (displayCycleStep == 0) {
      clearDisplay(VOLTAGE_DISP_OFFSET);
    } else {
      displayVoltage(targetVoltage);
    }
  } else {
    displayVoltage(voltageMeasurement);
  }
  
  // Current display
  if (currentDisplayMode == setting) {
    if (displayCycleStep == 0) {
      clearDisplay(CURRENT_DISP_OFFSET);
    } else {
      displayCurrent(targetCurrentLimit);
    }
  } else {
    displayCurrent(currentMeasurement);
  }
  
  // Advance display cycle
  displayCycleStep += 1;
  if (displayCycleStep == DISPLAY_CYCLES) displayCycleStep = 0;
}

/**
 * Shows a given float voltage value on the relevant display.
 * 
 * Displays in format [X]X.XX
 * @todo Fix bug where first digit remains when no update is available
 *       Should be blanked instead.
 */
void displayVoltage(float voltage) {
  float roundedVoltage = roundToNearest(voltage, VOLTAGE_STEP);
  signed char digits[DISP_DIGITS];
  prepareForDisplay(roundedVoltage, VOLTAGE_DECIMAL_POINTS, digits);
  writeDisplay(VOLTAGE_DISP_OFFSET, VOLTAGE_DECIMAL_POINTS, digits);
}

/**
 * Shows a given float current value on the relevant display.
 *
 * Display it in format X.XXX
 */
void displayCurrent(float current) {
  float roundedCurrent = roundToNearest(current, CURRENT_STEP);
  signed char digits[DISP_DIGITS];
  prepareForDisplay(roundedCurrent, CURRENT_DECIMAL_POINTS, digits);
  writeDisplay(CURRENT_DISP_OFFSET, CURRENT_DECIMAL_POINTS, digits);
}

void writeDisplay(char displayOffset, char decimalPoints, signed char* digits) {
  char i;
  disp.shutdown(0, true);
  
  for(i = 0; i < DISP_DIGITS; i += 1) {     
    disp.setDigit(
      0,
      i + displayOffset,
      (digits[i] == -1 ? ' ' : digits[i]),
      i == DISP_DIGITS - (decimalPoints + 1)
    );
  }
  
  disp.shutdown(0, false);
}

/**
 * Clear a single display by writing space characters to every digit
 */
void clearDisplay(char displayOffset) {
  char i;
  disp.shutdown(0, true);
  
  for(i = 0; i < DISP_DIGITS; i++) disp.setDigit(0, i + displayOffset, ' ', false);
  
  disp.shutdown(0, false);
}

/**
 * Rounds a float value to the nearest <rounding>
 * 
 * Used to round voltage and current measurements to 0.05V and 0.005A
 *
 * @note Only works somewhat reliably with roundings ending in 5
 * @todo Investigate using some stdlib stuff to make this neater,
 *       yet still not riddled with floating point errors.
 */
float roundToNearest(float input, float rounding) {
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

  if (roundingRemainder >= ((rounding * preMultiplier) / 2)) roundingLeftOver += 1;

  int result = (intDivided * (rounding * preMultiplier)) + (roundingLeftOver * (rounding * preMultiplier)); 
  return (float)result / preMultiplier;
}

/**
 * Retrieves digits from a float value, in an array of chars
 * 
 * Used for preparing to display float values on 7-segment displays
 */
void prepareForDisplay(float input, char decimalPoints, signed char* outDigits) {
  int intInput = (int)(((int)(input * power(10, decimalPoints))) % power(10, DISP_DIGITS));
  char i;
  signed char digit;
  for (i = DISP_DIGITS - 1; i >= 0; i--) {
    digit = (signed char)(((intInput % power(10, i + 1)) - (intInput % power(10, i))) / power(10, i));
    // Don't display a digit at [X]x.xx if [X] is 0
    if (digit == 0 && i == (decimalPoints + 1)) digit = -1;
    outDigits[DISP_DIGITS - i - 1] = digit;
  }
}

/**
 * Integer implementation of pow
 */
int power(int base, int exp) {
  int i, result;

  if (exp == 0) return 1;

  result = base;

  for (i = 1; i < exp; i++) result *=  base;

  return result;
}

