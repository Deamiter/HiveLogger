#ifndef Onboard_h
#define Onboard_h

#include <stddef.h>
#include <avr/pgmspace.h>

class Onboard
{
	public:
		uint8_t _ReadCalibrationByte( uint8_t index )
		{
			uint8_t result;

			// Load the NVM Command register to read the calibration row. 
			NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
			result = pgm_read_byte(index);

			// Clean up NVM Command register. 
			NVM_CMD = NVM_CMD_NO_OPERATION_gc;

			return( result );
		};
		void setupBattSense(){
			PR.PRPB &= ~0x02;
			ADCB.CALL = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL0) );
			ADCB.CALH = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL1) );
			ADCB.CALL = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL0) );
			ADCB.CALH = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL1) );
		};
		void setupTempSense(){
			PR.PRPA &= ~0x02;
			ADCA.CALL = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0) );
			ADCA.CALH = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1) );
			ADCA.CALL = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0) );
			ADCA.CALH = _ReadCalibrationByte( offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1) );
		};


		float getBatt(){
			//enable MOSFET
			PORTF.DIR|=PIN7_bm;
			PORTF.OUT|=PIN7_bm;

			PORTB.DIR = 0;                                   // configure PORTA as input
			ADCB.CTRLA |= 0x1;                               // enable adc
			ADCB.CTRLB = ADC_RESOLUTION_12BIT_gc;            // 12 bit conversion
			ADCB.REFCTRL = ADC_REFSEL0_bm;                   //sets reference to VCC/2.0=3.3/2.0=1.65   
			// Set ADC clock to 125kHz:  CPU_per/64    =>    8MHz/64 = 125kHz
			ADCB.PRESCALER = ADC_PRESCALER2_bm;
			ADCB.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc; // single ended

			ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc;        // PORTA:1

			int sample = 0;
			for(int i=0; i<8; i++){
				ADCB.CH0.CTRL |= ADC_CH_START_bm;
			while(!ADCB.CH0.INTFLAGS);
				sample+=ADCB.CH0.RES;
			}
			sample>>=3;

			float resultADC = (sample/4056.0)*1.65;
			#ifdef DEBUG
			Serial.print("raw: ");
			Serial.print(resultADC);
			Serial.println("V");
			#endif		
  			//disable MOSFET
  			PORTF.OUT|=PIN7_bm;
 			return resultADC*12; //12 was figured out emperically... should be 7?!

		};

		float getTemp(){
			//PORTA.DIR = 0;                                   // configure PORTA as input
			ADCA.CTRLA |= 0x1;                               // enable adc
			ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;            // 12 bit conversion
			ADCA.REFCTRL = ADC_REFSEL0_bm;                   //sets reference to VCC/2.0=3.3/2.0=1.65   
			// Set ADC clock to 125kHz:  CPU_per/64    =>    8MHz/64 = 125kHz
			ADCA.PRESCALER = ADC_PRESCALER2_bm;
			ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc; // single ended
			ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;        // PORTA:7
			int sample = 0;

			//delay(100);
	  		for(int i=0; i<8; i++){
				
	    			ADCA.CH0.CTRL |= ADC_CH_START_bm;
	    			while(!ADCA.CH0.INTFLAGS);
	    			delay(5);
				sample+=ADCA.CH0.RES;
	  		}
	  		sample>>=3;
	  		float resultADC = (sample/4.056)*1.65; //convert to mV
			#ifdef DEBUG
			Serial.print("raw: ");
			Serial.print(resultADC);
			Serial.println("mV");
			#endif

			#define TEMPERATURE_COEFF 19.5
			#define V_0C 400
			resultADC = (resultADC-V_0C)/TEMPERATURE_COEFF;
			#ifdef DEBUG
			Serial.print("BOARD: ");
			Serial.print(resultADC);
			Serial.println("*C");
			Serial.println();
			#endif
			return resultADC;
		}
};

extern Onboard onboard;

#include "Sensorhub.h"

#define ONBOARD_TEMP_UUID 0x0001
#define ONBOARD_TEMP_LENGTH_OF_DATA 2
#define ONBOARD_TEMP_SCALE 100
#define ONBOARD_TEMP_SHIFT 50


class OnboardTemperature: public Sensor
{
	public:
                String getName(){ return "On-board temperature sensor"; }
                String getUnits(){ return "*C";}
                
		OnboardTemperature():Sensor(ONBOARD_TEMP_UUID, ONBOARD_TEMP_LENGTH_OF_DATA, ONBOARD_TEMP_SCALE, ONBOARD_TEMP_SHIFT){};
                void init(){  onboard.setupTempSense();  }

                void getData()
                {                  
                  float sample = onboard.getTemp();
                  uint16_t tmp = (sample + ONBOARD_TEMP_SHIFT) * ONBOARD_TEMP_SCALE;
                  data[1]=tmp>>8;
                  data[0]=tmp;
                }       
};

#define ONBOARD_BATT_UUID 0x0002
#define ONBOARD_BATT_LENGTH_OF_DATA 2
#define ONBOARD_BATT_SCALE 100
#define ONBOARD_BATT_SHIFT 0

class BatteryGauge: public Sensor
{
        public:
                String getName(){ return "Battery Voltage"; }
                String getUnits(){ return "V";}

                BatteryGauge():Sensor(ONBOARD_BATT_UUID, ONBOARD_BATT_LENGTH_OF_DATA, ONBOARD_BATT_SCALE, ONBOARD_BATT_SHIFT){};
                void init(){  onboard.setupBattSense();  }

                void getData()
                {
                  float sample = onboard.getBatt();
                  uint16_t tmp = (sample + ONBOARD_BATT_SHIFT) * ONBOARD_BATT_SCALE;
                  data[1]=tmp>>8;
                  data[0]=tmp;
                }
};


#endif
