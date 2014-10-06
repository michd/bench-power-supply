#include "LedControl.h"
#include "ClickEncoder.h"
#include "TimerOne.h"

// # Configuration
// Voltage control
#define COARSE_VOLTAGE_RANGE 30
#define FINE_VOLTAGE_RANGE 5
// Adds up to 35V
#define VOLTAGE_STEP 0.05
#define CURRENT_STEP 0.005
// TODO: add calibration values here

// Display
#define DISP_INTENSITY 0x8

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
}

void loop() {
  // This will actually do things at some point
}

/**
 * Reads voltage from analog input and translates it to the
 * actual voltage value.
 * 
 * Value from ADC ranges 0-1023, which maps to 0-39V at 
 * 0.038V precision.
 * 
 * Value is rounded to the nearest 0.05V.
 *
 * @todo implement.
 */
float readVoltage() {
  return 0.0;
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
 * @todo implement.
 */
float readCurrent() {
  return 0.0;
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
void writeVoltage() {
}

/**
 * Shows a given float voltage value on the relevant display.
 * 
 * Should display it in format [X]X.XX
 *
 * Can probably use a common helper method for actual displaying.
 * 
 * @note This implementation will need extensive testing.
 * @todo Implement.
 */
void displayVoltage(float voltage) {
}

/**
 * Shows a given float current value on the relevant display.
 *
 * Should display it in format X.XXX
 *
 * Can probably use a common helper method for actual displaying.
 *
 * @note This implementation will need extensive testing
 * @todo Implement.
 */
void displayCurrent(float current) {
}


