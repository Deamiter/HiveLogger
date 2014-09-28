
// HX711 driver for use with Apitronics Bee (see Apitronics.com for more information on the platform).
// Based on the C Reference Driver in the Avia Semiconductor HX711 datasheet.
// I don't recommend using this if you know how to code properly. The pins used should certainly not be hard-coded into the driver!

#define AHX711_h

#define AHX711_UUID 0x00000030
#define AHX711_LENGTH_OF_DATA 3
#define AHX711_SCALE 745
#define AHX711_SHIFT 5000

#define CLOCKPIN A0
#define DOUTPIN A1

class AHX711: public Sensor
{
	public:
		AHX711(uint8_t samplePeriod=1):Sensor(AHX711_UUID, AHX711_LENGTH_OF_DATA, AHX711_SCALE, AHX711_SHIFT){};
		String getName() { return "Hive Weight";}
		String getUnits(){ return "pounds";}
		void init(){
			
			pinMode(CLOCKPIN, OUTPUT);
			pinMode(DOUTPIN, INPUT);
		}
			
		void getData() {
			// Turn Scale On
			digitalWrite(CLOCKPIN, LOW);
			// digitalWrite(CLOCKPIN, HIGH);
			
				
			uint32_t HX711Data;
			unsigned char i;
			
			while (digitalRead(DOUTPIN));			


	
			// pulse the clock pin 24 times to read the data 
			for (i=0; i<24; i++) {
				digitalWrite(CLOCKPIN, HIGH);
				HX711Data = HX711Data<<1;
				HX711Data = HX711Data + digitalRead(DOUTPIN);
				digitalWrite(CLOCKPIN, LOW);
			}
			
			
			// Turn Scale Off
			// digitalWrite(CLOCKPIN, HIGH);
			// digitalWrite(CLOCKPIN, LOW);
	
			HX711Data = 1-HX711Data;
			// The data needs to be formatted the way it should be displayed (above).
			// Then it should be scaled and shifted with AHX711_SCALE and AHX711_SHIFT to fit into an unsigned int.
			
			uint32_t tmp = HX711Data;
			data[2]=tmp>>16;
			data[1]=tmp>>8;
			data[0]=tmp;
		}
};



		