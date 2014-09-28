#include <stdint.h>
#include <avr/sleep.h>
#include "RTClib.h"
#include "Wire.h"

#include "Clock.h"

#define SET_BIT(p,n) ((p) |= (1 << (n)))
#define CLR_BIT(p,n) ((p) &= (~(1) << (n)))

//#define DEBUG

//CONSTRUCTORS
////////////////////////////////////////////////////////////////////////////
Clock::Clock(){
	_semaphore= true;
}

//PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////
void Clock::I2Cenable(){
	I2C_BEGIN(2,8);
}

void Clock::begin(DateTime date)
{
	if(_initialized) return;
	_initialized=true;
	I2Cenable();
	while(!I2C_READY());
     	// select status register
     	I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
     	I2C_WRITE(STATUS_REG);
     	I2C_END_TRANSMIT();
    
     	// read status register
     	I2C_REQUEST(DS1339_I2C_ADDRESS,1);
     	while(!I2C_READY());

     	// OSF flag will tell us if clock has been stopped
	//uint8_t ctl = I2C_READ();
	//Serial.println(ctl,BIN);
	//Serial.println(ctl>>OSCILLATOR_STOP_FLAG);
	


	if(I2C_READ()>>OSCILLATOR_STOP_FLAG){
       		setDate(date);
     	}

	uint8_t control = 0b0010000;
	while(!I2C_READY());
  	//write new control uint8_t back
  	I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  	I2C_WRITE(CONTROL_REG);
  	I2C_WRITE(control);
  	I2C_WRITE((uint8_t)0x00); //clears status register
  	I2C_END_TRANSMIT();

	//enable alarm interrupt output on clock
	enableAlarm1();

	//set PF2 as input
	PORTF.DIRCLR = 0x4;	//configure PF2 as input
	PORTF_PIN2CTRL = (0b011<<3)|0b010; //use internal pull-up and sense any edge

	PORTF.DIRCLR = 0b01000000;	//configure PF6 as input
	PORTF_PIN6CTRL = (0b011<<3)|0b010; //use internal pull-up and sense any edge
}

void Clock::I2Cdisable(){
	TWIE.MASTER.CTRLB = 0;
	TWIE.MASTER.BAUD = 0;
	TWIE.MASTER.CTRLA = 0;
	TWIE.MASTER.STATUS = 0;
}

//uses UNIX time from compilation to set time
void Clock::setDate(DateTime date)
{
   //If I could force recompile of this library, I could avoid passing this explicity
   second = date.ss; // Use of (uint8_t) type casting and ascii math to achieve result.
   minute = date.mm;
   hour = date.hh;
   dayOfWeek = 0;
   dayOfMonth = date.d;
   month = date.m;
   year= date.yOff;

   _writeDateToEEPROM();
}

void Clock::_writeDateToEEPROM(){
   
   I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
   I2C_WRITE((uint8_t)0x00);
   I2C_WRITE(decToBcd(second) & 0x7f); // 0 to bit 7 starts the clock
   I2C_WRITE(decToBcd(minute));
   I2C_WRITE(decToBcd(hour)); // If you want 12 hour am/pm you need to set
                                   // bit 6 (also need to change readDateDS1339)
   I2C_WRITE(decToBcd(dayOfWeek));
   I2C_WRITE(decToBcd(dayOfMonth));
   I2C_WRITE(decToBcd(month));
   I2C_WRITE(decToBcd(year));
   I2C_END_TRANSMIT();

}

//uses String input to set time
//eg: "Wed, 07 May 2014 12:54";
void Clock::setDate(String date)
{
   getDate();
   Serial.print("given date: ");
   Serial.println(date);
   //day of week is user define
   String daysOfWeek[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sun", "Mon"};
   String monthsOfYear[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
   for(uint8_t day=0; day<7; day++){
      if ( date.substring(0,3)==(daysOfWeek[day]) ) {
          //Serial.print(daysOfWeek[day]); Serial.print(": "); Serial.println(day);
          dayOfWeek=day+1;
          break;
      }
   }

   
   
   for(uint8_t m=0; m<12; m++){
      if ( date.substring(8,11)==(monthsOfYear[m]) ) {
          //Serial.print(monthsOfYear[m]); Serial.print(": "); Serial.println(m+1);
	  month=m+1;
	  break;
      }
   }

   dayOfMonth = atoi(&date[5]);
   //Serial.print("Day of month: ");
   //Serial.println(dayOfMonth);

   year = atoi(&date[13]);
   //Serial.print("Year: ");
   //Serial.println(year);

   hour = atoi(&date[17]);
   //Serial.print("hour: ");
   //Serial.println(hour);

   minute = atoi(&date[20]);
   //Serial.print("minute: ");
   //Serial.println(minute);

   _writeDateToEEPROM();
}


void Clock::getDate()
{
  int date[7];
  // Reset the register pointer
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE((uint8_t)0x00);
  I2C_END_TRANSMIT();
  
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS, 7);
 
  for(int i=0;i<7;i++){
    while(!I2C_READY());
    date[i]= I2C_READ();
    
  }
  // A few of these need masks because certain bits are control bits
  second = bcdToDec(date[0] & 0x7f);
  minute = bcdToDec(date[1]);
  hour = bcdToDec(date[2] & 0x3f); // Need to change this if 12 hour am/pm
  dayOfWeek = bcdToDec(date[3]);
  dayOfMonth = bcdToDec(date[4]);
  month = bcdToDec(date[5]);
  year = bcdToDec(date[6]);
}

void Clock::print(){
getDate();
_print();
}

String Clock::timestamp(){
	getDate();
	String output = "";   
	if (hour < 10)   output+="0";
   	output+=String(hour);
   	output+=":";
   	if (minute < 10)   output+="0";
	output+=String(minute);
	output+=":";
	if (second < 10)   output+="0";
	output+=String(second);
	output+=", ";

	if (dayOfMonth < 10) output+="0";
	output+=String(dayOfMonth);
	output+="/";
	if (month<10) output+="0";
	output+=String(month);
	output+="/";
	if (year<10) output+="0";
	output+=year;
	return output;
}


void Clock::_print(){
   Serial.println(timestamp());
}
void Clock::enableAlarm1(){
  //first retrieve control uint8_t
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(0xE);
  I2C_END_TRANSMIT();
 
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS, 1);
  delay(2);

  uint8_t control=0;
  while(!I2C_READY());

  control = I2C_READ();
  control = control|0b0000101; //enables A1IE and INTCN, alarm 1 and interrupt pin toggle respectively
  //OR it with current control uint8_t to not clear other bits

  
  while(!I2C_READY());
  //write new control uint8_t back
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(CONTROL_REG);
  I2C_WRITE(control);
  I2C_WRITE((uint8_t)0x00); //clears status register
  I2C_END_TRANSMIT();
}

void Clock::enableAlarm2(){
  //first retrieve control uint8_t
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(0xE);
  I2C_END_TRANSMIT();
 
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS, 1);
  delay(2);

  uint8_t control=0;
  while(!I2C_READY());

	
  control = I2C_READ();
  control = control|0b0000110; //enables A2IE and INTCN, alarm 2 and interrupt pin toggle respectively
  //control = 0b110; //OR it with current control uint8_t to not clear other bits

  
  while(!I2C_READY());
  //write new control uint8_t back
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(CONTROL_REG);
  I2C_WRITE(control);
  I2C_WRITE((uint8_t)0x00); //clears status register
  I2C_END_TRANSMIT();
}



void Clock::setAlarm1Delta(uint8_t minutesAsleep, uint8_t secondsAsleep){
  _semaphore= false;
  getDate();

  while(!I2C_READY());
  //this puts the highest bit (bit 7) of seconds, minutes, hours, and day to HIGH
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_1_REG); // This would cause an alarm every second
  I2C_WRITE((uint8_t)0x80); // but they will be reconfigured in a few lines
  I2C_WRITE((uint8_t)0x80);
  I2C_WRITE((uint8_t)0x80);
  I2C_WRITE((uint8_t)0x80);
  I2C_END_TRANSMIT();
 
  
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE((uint8_t)(0x00)); //set pointer to get time
  I2C_END_TRANSMIT();
  
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS, 2); //get minutes
  while(!I2C_READY());
  //find next even SECONDS_ASLEEP second time
  test = bcdToDec(I2C_READ() & 0x7f); //get reading and convert
  while( ++test % secondsAsleep!= 0 ); //increment until multiple of seconds asleep
  test%=60;
  if(secondsAsleep>0){
    second = test%10 | ((test/10)<<4); //bits [0:3] are seconds less then 10, bits [4:6] are 10s of seconds
  }
  else
    second = 0x00; //the default is to set alarm at top of the minute
 
  //same routine down here but for minutes
  test = bcdToDec(I2C_READ());
  while( ++test % minutesAsleep != 0 );
  test%=60;
  
  if(minutesAsleep>0)
    minute = test%10 | ((test/10)<<4);
  else
    minute = 0x80; //the default is to not care what minute it is (highest bit is 1)

  while(!I2C_READY());
  
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_1_REG); //set pointer
  I2C_WRITE(second); //write to registers
  I2C_WRITE(minute);
  I2C_END_TRANSMIT();
  
  _clearStatusRegister();
    
  #ifdef DEBUG
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_1_REG);
  I2C_END_TRANSMIT();
  
  I2C_REQUEST(DS1339_I2C_ADDRESS,4);
  while(!I2C_READY());
  Serial.print("Alarm scheduled for ");
  Serial.print("SEC: ");
  Serial.print(bcdToDec(I2C_READ() & 0x7f));
  Serial.print(" MIN: ");
  Serial.print(bcdToDec(I2C_READ()&0x7F));
  Serial.print(" HR: ");
  Serial.print(bcdToDec(I2C_READ()&0x3f));
  Serial.print(" DAY: ");
  Serial.println(bcdToDec(I2C_READ()&0x7F));
  
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(CONTROL_REG);
  I2C_END_TRANSMIT();
 

  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS,2);
  Serial.print("CONTROL: ");
  Serial.println(I2C_READ(),BIN);
  Serial.print("STATUS: ");
  Serial.println(I2C_READ(),BIN);

  Serial.println("Alarm 1 Configured");
  #endif

  
}

void Clock::setAlarm2Delta(uint8_t minutesAsleep){
  _semaphore= false;
  getDate();
  
  while(!I2C_READY());
  //this puts the highest bit (bit 7) of seconds, minutes, hours, and day to HIGH
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_2_REG); 
  // This would cause an alarm every second
  I2C_WRITE((uint8_t)0x80); // but they will be reconfigured in a few lines
  I2C_WRITE((uint8_t)0x80);
  I2C_WRITE((uint8_t)0x80);
  I2C_END_TRANSMIT();
 
  
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE((uint8_t)(0x00)); //set pointer to get time
  I2C_END_TRANSMIT();

  uint8_t secondsAsleep=0x5;
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS, 3); //get minutes & hours

  while(!I2C_READY());
  //find next even SECONDS_ASLEEP second time
  test = bcdToDec(I2C_READ() & 0x7f); //get reading and convert
  while( ++test % secondsAsleep!= 0 ); //increment until multiple of seconds asleep
  test%=60;
  if(secondsAsleep>0){
    second = test%10 | ((test/10)<<4); //bits [0:3] are seconds less then 10, bits [4:6] are 10s of seconds
  }
  else
    second = 0x00; //the default is to set alarm at top of the minute
 
  //same routine down here but for minutes
  test = bcdToDec(I2C_READ());
  while( ++test % minutesAsleep != 0 );
  test%=60;
  
  uint8_t hoursAsleep = minutesAsleep/60;
  uint8_t minuteAsleep = minutesAsleep%60;

  if(minutesAsleep>0)
    minute = test%10 | ((test/10)<<4);
  else
    minute = 0x80; //the default is to not care what minute it is (highest bit is 1)

  test = bcdToDec(I2C_READ()); 		//test reads current hour
  while( ++test % hoursAsleep != 0 ); 	//increment test and divide it by hours
  test%=24;				//make sure it's base 24!
  if(hoursAsleep>0){
    Serial.println(test%10 | ((test/10)<<4), BIN);
    hour = test%10 | ((test/10)<<4);
  }
  else	hour = 0x80; //the default is to not care what minute it is (highest bit is 1)

  while(!I2C_READY());
  
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_2_REG); //set pointer
  I2C_WRITE(minute);
  I2C_WRITE(hour);
  I2C_END_TRANSMIT();
  
  _clearStatusRegister();
    
  #ifdef DEBUG
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(ALARM_2_REG);
  I2C_END_TRANSMIT();
  
  I2C_REQUEST(DS1339_I2C_ADDRESS,3);
  while(!I2C_READY());
  Serial.print("Alarm 2 scheduled for ");
  Serial.print(" MIN: ");
  Serial.print(bcdToDec(I2C_READ()&0x7F));
  Serial.print(" HR: ");
  Serial.print(bcdToDec(I2C_READ()&0x3f));
  Serial.print(" DAY: ");
  Serial.println(bcdToDec(I2C_READ()&0x7F));
  
  while(!I2C_READY());
  I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
  I2C_WRITE(CONTROL_REG);
  I2C_END_TRANSMIT();
  
  while(!I2C_READY());
  I2C_REQUEST(DS1339_I2C_ADDRESS,2);
  Serial.print("CONTROL: ");
  Serial.println(I2C_READ(),BIN);
  Serial.print("STATUS: ");
  Serial.println(I2C_READ(),BIN);
  #endif

  #ifdef DEBUG
  Serial.println("Alarm 2 Configured");
  #endif

  
}


// Convert normal decimal numbers to binary coded decimal
uint8_t Clock::decToBcd(uint8_t val)
{
	return ( (val/10*16) + (val%10) );
}
 
// Convert binary coded decimal to normal decimal numbers
uint8_t Clock::bcdToDec(uint8_t val)
{
	return ( (val/16*10) + (val%16) );
}

bool Clock::alarmFlag(){
	I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
	I2C_WRITE(STATUS_REG);
	I2C_END_TRANSMIT();

	uint8_t data;
	I2C_REQUEST(DS1339_I2C_ADDRESS,1);
	delay(2);
	data = I2C_READ();
	I2C_END_TRANSMIT();
	return data&0b1 | data&0b10;
	
}

bool Clock::triggeredByA1(){
	return _flags&0b1;
}

bool Clock::triggeredByA2(){
	return _flags&0b10;
}



void Clock::getFlags(){
	I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
	I2C_WRITE(STATUS_REG);
	I2C_END_TRANSMIT();

	uint8_t data;
	I2C_REQUEST(DS1339_I2C_ADDRESS,1);
	delay(2);
	_flags = I2C_READ();
	I2C_END_TRANSMIT();	
}

void Clock::clearFlags(){
	_flags=0;
}

void Clock::_clearStatusRegister(){
	while(!I2C_READY());
	//clear flag before leaving
	I2C_BEGIN_TRANSMIT(DS1339_I2C_ADDRESS);
	I2C_WRITE(STATUS_REG);
	I2C_WRITE((uint8_t)0x00);
	I2C_END_TRANSMIT();
}

Clock clock;
