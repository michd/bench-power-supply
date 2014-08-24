#define COARSE_RANGE 10
#define FINE_RANGE 5

int coarsePin = 10;
int finePin = 11;
void setup() {
  pinMode(coarsePin, OUTPUT);
  pinMode(finePin, OUTPUT);
  setVoltage(0.15);
}

void loop() { }


void setVoltage(float voltage) {
  // coarse component becomes coarse by casting to int
  int coarse = (int) voltage;
  //fine will be what remains
  float fine;
  
  if (coarse > COARSE_RANGE) {
    coarse -= (coarse - COARSE_RANGE);
  }
  
  fine = voltage - (float) coarse;
  setCoarse(coarse);
  setFine(fine);
}

//0-10V, 1V precision
void setCoarse(int cVoltage) {
  //10V = 255
  // Convert voltage to 8 bit pwm output value in our voltage range
  int cAnalog = (int) ((255.0 / (float) COARSE_RANGE) * cVoltage);
  analogWrite(coarsePin, cAnalog);
}

//0-5V here, ~.02V precision
void setFine(float fVoltage) {
  // Convert voltage to 8 bit pwm output value in our voltage range
  int fAnalog = (int) ((255.0 / (float) FINE_RANGE) * fVoltage);
  analogWrite(finePin, fAnalog);
}

