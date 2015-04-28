
// HX711 driver for use with Apitronics Bee (see Apitronics.com for more information on the platform).
// Based on the C Reference Driver in the Avia Semiconductor HX711 datasheet.
// I don't recommend using this if you know how to code properly. The pins used should certainly not be hard-coded into the driver!

// This is a messy driver. Seriously, don't use it unless you're desperate or seriously can't code yourself!

#define AHX711_h

#define AHX711_UUID 0x00000030
#define AHX711_LENGTH_OF_DATA 3
#define AHX711_SCALE 745
#define AHX711_SHIFT 5000

class AHX711: public Sensor
{
	private:
		uint8_t _clockpin;
		uint8_t _doutpin;
	public:
		AHX711(uint8_t clockpin, uint8_t doutpin, uint8_t samplePeriod=1):Sensor(AHX711_UUID, AHX711_LENGTH_OF_DATA, AHX711_SCALE, AHX711_SHIFT){
			_clockpin=clockpin;
			_doutpin = doutpin;
			
		};
		String getName() { return "Hive Weight";}
		String getUnits(){ return "pounds";}
		void init(){
			
			pinMode(_clockpin, OUTPUT);
			pinMode(_doutpin, INPUT);
		}
			
		void getData() {
			// Turn Scale On
			digitalWrite(_clockpin, LOW);
			// digitalWrite(_clockpin, HIGH);
			
				
			uint32_t HX711Data;
			unsigned char i;
			
			while (digitalRead(_doutpin));			


	
			// pulse the clock pin 24 times to read the data 
			for (i=0; i<24; i++) {
				digitalWrite(_clockpin, HIGH);
				HX711Data = HX711Data<<1;
				HX711Data = HX711Data + digitalRead(_doutpin);
				digitalWrite(_clockpin, LOW);
			}
			
			
	
			HX711Data = 1-HX711Data;
// This is just a cheap trick to flip the slope of voltage with weight on the load cell. This should NOT be necessary in any final version of this driver -- scaling should (probably) either be handled fully in the driver, or more likely with scaling factors that can be set to scale the HX711 output to actual weight.


			// The data needs to be formatted the way it should be displayed (above).
			// Then it should be scaled and shifted with AHX711_SCALE and AHX711_SHIFT to fit into an unsigned int.
			
// Notice that I'm not using AHX711_SCALE and AHX711_shift. That's because I ran out of time to implement them properly. Again, this driver is incomplete, even if it DOES technically collect useful data from the HX711 chip.

			uint32_t tmp = HX711Data;
			data[2]=tmp>>16;
			data[1]=tmp>>8;
			data[0]=tmp;
		}
};



		
