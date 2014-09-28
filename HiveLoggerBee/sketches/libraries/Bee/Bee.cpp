#include "Wire.h"
#include "avr/sleep.h"

//#include "XBeePlus.h"
#include "Clock.h"
#include "RTClib.h"
#include "Onboard.h"
#include "Bee.h"

enum
{
	IDLE,
	PWR_DOWN,
	PWR_SAVE
}state;

#define LOW_LVL_INT 0b1

void configureSleep(){
	onboard.setupBattSense();	
	cli();
	//PORTF.DIRCLR=PIN2_bm;
	PORTF_INT0MASK = PIN2_bm; // give PIN2 the INT0 interrupt
	PORTF_INTCTRL |= LOW_LVL_INT; // makes INT0 a low level interrupt
	PMIC_CTRL |= LOW_LVL_INT; //globally enable low level interrupts
	sei();
}

//TODO: repeated push button can cause a state where Bee goes to sleep with PF2 pulled low by clock 
//	==> clock and push button can no longer wake the device
void sleep(){
	state = PWR_DOWN;
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	while(!I2C_READY()); //important to stay here until alarm is configured to avoid going to sleep
	cli();
	sleep_enable();
	
	sei();
	_goToSleep();

	sleep_disable();

	clock.getFlags();
	
	//battery needs to be protected from overextension	
	while(onboard.getBatt()<8.5) {	
		Serial.println("I have low battery!");
		clock.setAlarm1Delta(0,5);

		state = PWR_DOWN;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		while(!I2C_READY()); //important to stay here until alarm is configured to avoid going to sleep
		cli();
		sleep_enable();

		sei();
		_goToSleep();

		sleep_disable();
		clock.clearFlags();
	}
}

void _goToSleep(){
	delay(1);
	sleep_cpu();
	if(state==PWR_DOWN){
		Serial.println("waah");
		_goToSleep();
	}
	else{
		return;
	}
}

ISR(PORTF_INT0_vect){
	state=IDLE;
}

volatile uint32_t _count=0;

void configureSleepDelay(){
   cli();
   RTC_INTCTRL|=0b1100;
   sei();
}

volatile uint16_t sleepMillis = 0;

void sleepDelay(uint16_t millis){
  sleepMillis=millis;
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  cli();
  sleep_enable();
  sei();
  byte prev = RTC_INTCTRL;
  RTC_INTCTRL=0b1100;
  _goToSleep2();
  sleep_disable();
  RTC_INTCTRL=prev;
  _count=0;
}

void _goToSleep2(){
  while(_count*4<sleepMillis){ 
    sleep_cpu();
  }
}

ISR(RTC_COMP_vect){
  _count++;
}
