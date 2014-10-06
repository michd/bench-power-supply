#include "LedControl.h"
#include "ClickEncoder.h"

#define COARSE_VOLTAGE_RANGE 30
#define FINE_VOLTAGE_RANGE 5

// Pins
// I/O board
#define O_DISP_DATA_IN 12
#define O_DISP_CLOCK   11
#define O_DISP_LOAD    10
#define I_SW_VOLTAGE   8
#define I_ROTA_VOLTAGE A2
#define I_ROTB_VOLTAGE A3
#define I_SW_CURRENT   9
#define I_ROTA_CURRENT A4
#define I_ROTB_CURRENT A5

// Voltage / current measuring
#define AI_MEASURE_CURRENT A1
#define AI_MEASURE_VOLTAGE A0

// Voltage control PWM
#define PWMO_VOLTAGE_COARSE 5
#define PWMO_VOLTAGE_FINE   6



