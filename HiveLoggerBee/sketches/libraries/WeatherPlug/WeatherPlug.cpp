#include "Sensorhub.h"
#include "Clock.h"
#include "Wire.h"
#include "WeatherPlug.h"
#include "Bee.h"

#define I2C_BEGIN xmWireC.begin
#define I2C_BEGIN_TRANSMIT xmWireC.beginTransmission
#define I2C_WRITE xmWireC.write
#define I2C_REQUEST xmWireC.requestFrom
#define I2C_READ xmWireC.receive
#define I2C_READY xmWireC.ready
#define I2C_END_TRANSMIT xmWireC.endTransmission
#define I2C_DISABLE disableI2C
#define I2C_DELAY 3

#include <avr/io.h>
#include <avr/interrupt.h>

////////////////////////////////////////////////////////////////////////
//	Weather Plug shared
////////////////////////////////////////////////////////////////////////
void WeatherPlug::init(){
	if (initialized) return; 
	
	clock.begin(DateTime(__DATE__, __TIME__));
	Serial.println("initializing");

		  
  	pinMode(5,OUTPUT);
  	digitalWrite(5,HIGH);

	cli();
	//pinMode(7,INPUT);
        //pinMode(7,LOW);
      	PORTB_INT0MASK = 0b100; // give PIN2 the PORTB INT0 interrupt
        PORTB_INTCTRL = 0b1; // makes PORTB INT0 a low level interrupt
	
        PORTD_INT0MASK = 0b100; // give PIN2 the PORTD INT0 interrupt
       	PORTD_INTCTRL = 0b1; // makes PORTD INT0 a low level interrupt
	PMIC_CTRL |= 0b1; //globally enable low level interrupts  
	 
	sei();
	
	I2C_BEGIN();
	i2cChannel(1);

	initialized=true;
};
void WeatherPlug::i2cChannel(uint8_t channel){
	delay(I2C_DELAY);
	while(!I2C_READY());
	I2C_BEGIN_TRANSMIT(PCA9540);
	I2C_WRITE((byte) 1<<channel );
	I2C_END_TRANSMIT(); 
	delay(I2C_DELAY);
};

//if we re enable I2C via the library, we'll waste a lot of memory!
void WeatherPlug::enableI2C(){
	unsigned long twiSpeed=0; 
	unsigned long twiBaudrate=0;
  
	PORTC.PIN0CTRL = 0x38;    
	PORTC.PIN1CTRL = 0x38;
  
	TWIC.MASTER.CTRLA = TWI_MASTER_INTLVL_LO_gc 
		| TWI_MASTER_RIEN_bm 
		| TWI_MASTER_WIEN_bm 
		| TWI_MASTER_ENABLE_bm;
  	twiSpeed=400000UL;
	twiBaudrate=(F_CPU/(twiSpeed<<2))-5UL;
	TWIC.MASTER.BAUD=(uint8_t)twiBaudrate;
	TWIC.MASTER.STATUS=TWI_MASTER_BUSSTATE_IDLE_gc;  
};
void WeatherPlug::disableI2C(){
	TWIC.MASTER.CTRLB = 0;
	//TWIC.MASTER.BAUD = 0;
	TWIC.MASTER.CTRLA = 0;
	//TWIC.MASTER.STATUS = 0;
};

void WeatherPlug::wake(){
	digitalWrite(5,HIGH);
	enableI2C();
	i2cChannel(1);
	bmp.begin();
	isAwake=true;
}

void WeatherPlug::sleep(){
	digitalWrite(5,LOW);
	disableI2C();
	isAwake=false;
}

//ADC Drivers
uint16_t WeatherPlug::_getADC(uint16_t channel){

//	digitalWrite(5,HIGH);
//        enableI2C();
  //      i2cChannel(1);

        I2C_BEGIN_TRANSMIT(ADS7828);
        I2C_WRITE((byte)0b01);
        I2C_END_TRANSMIT();
        delay(I2C_DELAY);

        I2C_BEGIN_TRANSMIT(ADS7828);
        I2C_WRITE((byte)0b01);
	byte channelSel = 0b11000101 | (channel<<4);
        I2C_WRITE((byte)channelSel);
        I2C_WRITE((byte)0b10000011);
        I2C_END_TRANSMIT();
        delay(I2C_DELAY);

        I2C_BEGIN_TRANSMIT(ADS7828);
        I2C_WRITE((byte)0b01);
        I2C_END_TRANSMIT();
        delay(I2C_DELAY);


        I2C_BEGIN_TRANSMIT(ADS7828);
        I2C_WRITE((byte)0b00);
        I2C_END_TRANSMIT();
        delay(I2C_DELAY);

        I2C_REQUEST(ADS7828,2);
        delay(I2C_DELAY);
	uint16_t data = I2C_READ()<<8;
        data |=I2C_READ();
	return data;
}

WeatherPlug weatherPlug;


////////////////////////////////////////////////////////////////////////
//	WIND DIRECTION DRIVERS
////////////////////////////////////////////////////////////////////////
uint16_t WeatherPlug::windDirCutoffs[NUM_WIND_CUTOFFS] = {46,   88, 144, 201,   244, 339,   465, 621,  754,   844, 1038,  1341, 1658,  1847, 1995};
float WeatherPlug::windDirMappings[NUM_WIND_MAPPINGS] =   {270, 315,   0, 337.5, 225, 247.5,  45, 22.5, 292.5, 180, 202.5,  135, 157.5,   90, 67.5, 112.5};

float WeatherPlug::mapWindDirection(uint16_t val){
	for(int i=0;i<15;i++)   if(val<windDirCutoffs[i])   return windDirMappings[i];
	return windDirMappings[15];
};

float WeatherPlug::getWindDirection(){
	uint16_t data = _getADC(3);
	data >>=4;
	return mapWindDirection(data); 
};

//SENSORHUB WRAPPERS
String WindDirection::getName(){ return "Wind Direction"; }
String WindDirection::getUnits(){ return " degrees";}
 
void WindDirection::init(){ weatherPlug.init();   }

void WindDirection::getData(){
	float sample = weatherPlug.getWindDirection();
	uint16_t tmp = (sample + WIND_DIR_SHIFT) * WIND_DIR_SCALE;
	data[1]=tmp>>8;
	data[0]=tmp;
}

////////////////////////////////////////////////////////////////////////
//	WIND SPEED DRIVERS
////////////////////////////////////////////////////////////////////////
//	note the ISR below this class

void WeatherPlug::startWind(){
	clock.getDate();
	windCount=0;
	W_sec = clock.second;
	W_min = clock.minute;
	W_hour = clock.hour;
}

// 1 click / second = 2.4 km/h
// this function returns a scalar so that clicks * coeff = km/hour
float WeatherPlug::getWindCoeff(){
	clock.getDate();
	uint16_t second = clock.second - W_sec;
	uint8_t minute = clock.minute - W_min;
	uint8_t hour = clock.hour - W_hour;
  	if(hour<0) hour = 24 + hour;
	uint16_t secondsElapsed = (hour*60 + minute)*60 + second;	
  	float coeff = 2.4/(secondsElapsed) ;
  	return coeff;
}

float WeatherPlug::getWindSpeed(){
	return windCount*getWindCoeff();

}

//SENSORHUB WRAPPERS
String WindSpeed::getName() { return "Wind Speed"; }
String WindSpeed::getUnits() { return "km/h"; }

void WindSpeed::init()
{
	weatherPlug.init();
	weatherPlug.startWind();
};
void WindSpeed::getData(){
	uint32_t tmp = weatherPlug.getWindSpeed()*WIND_SPEED_SCALE;
        data[3]=tmp>>24;
	data[2]=tmp>>16;
	data[1]=tmp>>8;
        data[0]=tmp;
	weatherPlug.startWind();
};


//DRIVERS
ISR(PORTD_INT0_vect){
  weatherPlug.windCount++;
};

float WeatherPlug::getRainfall(){
	uint32_t count = rainCount/8.0;
	rainCount=0;
	float ret = count * 0.2794;
	return ret;
}


//SENSORHUB WRAPPERS
String Rainfall::getName() { return "Rain Fall"; };
String Rainfall::getUnits() {return "mm";};

void Rainfall::init() { weatherPlug.init(); }

void Rainfall::getData(){
    uint16_t rain = (weatherPlug.getRainfall()+getShift()) * getScalar();
    //unsual shifts here to divide by two
    data[3]=rain>>24; 
    data[2]=rain>>16;
    data[1]=rain>>8;
    data[0]=rain; 
}


ISR(PORTB_INT0_vect){
  weatherPlug.rainCount++;
};

//SENSORHUB WRAPPER FOR BMP TEMPERATURE

String BMP_Temperature::getName() { return "BMP Temperature"; };
String BMP_Temperature::getUnits() {return "*C";};

void BMP_Temperature::init() {weatherPlug.init(); }

void BMP_Temperature::getData(){
    	if(!weatherPlug.isAwake) weatherPlug.wake();
    	uint16_t temp = (weatherPlug.bmp.readTemperature()+getShift()) * getScalar();
    	//uint16_t rain=(0xFFFF*getScalar()+getShift());
	data[1]=temp>>8;
    	data[0]=temp;
};

//SENSORHUB WRAPPER FOR BMP PRESSURE

String BMP_Pressure::getName() { return "BMP Pressure"; };
String BMP_Pressure::getUnits() {return "P";};

void BMP_Pressure::init() {weatherPlug.init(); }

void BMP_Pressure::getData(){
        if(!weatherPlug.isAwake) weatherPlug.wake();
        uint32_t pressure = (weatherPlug.bmp.readPressure()+getShift()) + getShift();
        data[2]=pressure>>16;
	data[1]=pressure>>8;
        data[0]=pressure;
};

////////////////////////////////////////////////////////////////////////
//      SHT2x DRIVERS
////////////////////////////////////////////////////////////////////////
float WeatherPlug::SHT_getTemp(){
	//START SHT TEMP
	I2C_BEGIN_TRANSMIT(SHT2x);
    	I2C_WRITE(0b11100011);
    	I2C_END_TRANSMIT();   
   
    	/////////////////////////////////////////////////////////////////
    	delay(1);
    	while(!I2C_READY());
    	//delay(250);
    	I2C_REQUEST(SHT2x, 2);
    	delay(2);
    	while(!I2C_READY());
    	delay(1);
    	uint16_t cur = (I2C_READ()<<8);
    	cur |= I2C_READ();
    	return -46.85+175.72*cur/65536.0;  
};

float WeatherPlug::SHT_getRH(){
	delay(5);
	//START SHT RH
    	I2C_BEGIN_TRANSMIT(SHT2x);
    	I2C_WRITE(0b11110101);
    	I2C_END_TRANSMIT();
	/////////////////////////////////////////////////////////////////
	//sleepDelay(250);
    	while(!I2C_READY());
    	delay(100);
    	I2C_REQUEST(SHT2x, 2);
    	delay(5);
    	while(!I2C_READY());
    	uint16_t cur = (I2C_READ()<<8);
    	delay(5);
    	cur |= I2C_READ();
    	return -6 + 125*(cur/65536.0);
};

//SENSORHUB WRAPPER FOR SHT2x temperature and RH

String SHT2x_temp::getName() { return "SHT2x Temperature"; };
String SHT2x_temp::getUnits() {return "*C";};

void SHT2x_temp::init() {weatherPlug.init(); }

void SHT2x_temp::getData(){
        if(!weatherPlug.isAwake) weatherPlug.wake();
        uint16_t temp = (weatherPlug.SHT_getTemp() + getShift()) * getScalar();
        data[1]=temp>>8;
        data[0]=temp;
};

String SHT2x_RH::getName() { return "SHT2x Relative Humidity"; };
String SHT2x_RH::getUnits() {return " %";};

void SHT2x_RH::init() {weatherPlug.init(); }

void SHT2x_RH::getData(){
        if(!weatherPlug.isAwake) weatherPlug.wake();
        uint16_t RH = (weatherPlug.SHT_getRH()+getShift()) * getScalar() ;
        data[1]=RH>>8;
        data[0]=RH;
};

