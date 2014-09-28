#ifndef Bee_h
#define Bee_h

#include "Wire.h"
#include "Sensorhub.h"
#include "avr/sleep.h"

void configureSleep();	
void sleep();
void _goToSleep();
void configureSleepDelay();
void sleepDelay(uint16_t millis);
void _goToSleep2();
void setupTempSense();
void setupBattSense();
float getTemp();
float getBatt();
uint8_t _ReadCalibrationByte(uint8_t index);

#endif
