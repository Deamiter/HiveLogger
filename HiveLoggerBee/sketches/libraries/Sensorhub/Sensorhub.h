#ifndef Sensorhub_h
#define Sensorhub_h

#include "Arduino.h"

//need to figure out how to allow instances to allocate these sizes
#define MAX_DATA_SIZE 32 //bytes
#define MAX_SENSORS 32 //sensors
#define UUID_WIDTH 2 //bytes

/*TODO:
	- flipping LSB and MSB is confusing (happens in sensorhub atm)
	- why can't we do 2^16 samples and only 2^15?
	- how to allow sensor instance to determine MAX_DATA_SIZE?
	- how to allow sensorhub instance to determine MAX_SENSORS?
*/ 

class Sensor
{
	public:
		Sensor(const uint16_t uuid, const uint8_t dataWidthBytes, const uint16_t scale, const uint16_t shift, bool logIsSample=false, uint16_t samplePeriod=1){
			_uuid = uuid;				// 16 bit UUID
			_size = dataWidthBytes; 		// width of data, in bytes
			_count=0;
			_scale=scale;
			_shift=shift;
			_logIsSample = logIsSample;
			_samplePeriod = samplePeriod;
		}
		void print(){
			Serial.print("Sensor UUID: ");
			Serial.println(_uuid);

			Serial.print("Data length: ");
			Serial.print(_size);
			Serial.println(" bytes");

			Serial.print("Samples every ");
			Serial.print(_samplePeriod);
			Serial.println(" time steps");
		};

		void sample(bool print=false){
			if(!_logIsSample){
				for(int i=0;i<_size;i++) data[i]=0;
				getData();
				if (print) printData();
				//byte-wise summing
				uint8_t carry=0;				
				//start with LSB and carry overflow up
				for(uint8_t i=0; i<_size+2; i++){
					//if we're past the size of data, just do addition and carry
					uint16_t tmp;
					if ( i >_size ) tmp = carry + _sum[i];
					else tmp = carry + _sum[i] + data[i]; 
					_sum[i]=tmp;	// sum is array of uint8_t so loses top byte
					carry = tmp>>8; // here we preserve top byte of tmp
				}
				_count++;
			}
			/*
			uint32_t result = 0;
			for(int16_t i=_size+1; i>=0;i--){
				result = result<<8 | _sum[i];
			}
			Serial.print("Result: ");
			Serial.println(result);*/
		};

		//simple averaging of the samples
		//debug set print to true
		void log(bool print=false){
			//we could do this averaging operation on the array itself
			//but right now let's assume the largest data is 32 bits
			if(!_logIsSample){
				uint32_t result = 0;
				for(int16_t i=_size+1; i>=0; i--){
					result = result<<8 | _sum[i]; 	//accumulate bytes shifting them upwards
					_sum[i]=0;			//zero out the sum as you go
				}
				
				result/=_count;			//and divide the big number
				
				_count=0;
				for (int i=0; i<_size; i++){
					data[i]= result >> 8*i;	//put the average piece out in data
				}
			}
			else getData();
			if(print) 
			{
			 printData(); 
			}
		};

		void getData(bool print){
			getData();
          	if(print){
	    		printData();
          	}
		};

		void printData(){
			Serial.print(getName()+ ": ");
			uint32_t tmp = 0;
			for(int i=_size-1; i>=0; i--) tmp = data[i] | tmp<<8;
			float output = tmp/float(_scale)  -  _shift;
			Serial.print(output);
			Serial.println(getUnits());
		}
		
			

		uint8_t data[MAX_DATA_SIZE];
		uint16_t getUUID() { return _uuid; }
		uint8_t getSize() { return _size; }
		uint16_t getScalar() { return _scale; }
		uint16_t getShift() { return _shift; }
		void setSamplePeriod(uint16_t samplePeriod) { _samplePeriod=samplePeriod;}
		uint16_t getSamplePeriod() { return _samplePeriod; }	
	
		virtual void init();
		virtual void getData();
		virtual String getName();
		virtual String getUnits();	
	private:
		uint16_t _uuid;
		uint8_t _size; 
		uint8_t _sum[MAX_DATA_SIZE+2];
		uint16_t _count;
		uint16_t _scale;
		uint16_t _shift;
		bool _logIsSample;
		uint16_t _samplePeriod;
};


class Sensorhub
{
	private:
		Sensor** sensors;
		uint8_t _size;
		uint8_t sizeOfData;
		const uint8_t _findTotalSize(){
			uint8_t dataSize=0;
			for (int i=0; i<_size;i++){
				dataSize+=sensors[i]->getSize();
			}
			return dataSize;
		}

	public:
		Sensorhub(Sensor** sensorPointers, uint8_t numSensors){
			sensors = sensorPointers;
			_size = numSensors;
			sizeOfData=_findTotalSize();
			_count=0;
			/*sizeOfData[&]{
	                        const uint8_t sizeOfData = 0;
				uint8_t dataSize=0;
        	                for (int i=0; i<_size;i++) dataSize+=sensors[i]->getSize();
				sizeOfData = dataSize;
                        }*/
			//sizeOfData=0;

			
			for(uint8_t i=0; i<_size; i++){
				uint16_t uuid = sensors[i]->getUUID();
				ids[i*UUID_WIDTH]=uuid<<8;
				ids[i*UUID_WIDTH+1]=uuid;
			}
		}

		void init(){
			for (int i=0; i<_size;i++) sensors[i]->init();
		}
		//aug 29 - changed print to true for debug
		void sample(bool print=true){
			for (int i=0; i<_size;i++){
				if (_count%sensors[i]->getSamplePeriod()==0)
					sensors[i]->sample(print);
			}
			_count++;
		}

		uint8_t data[MAX_DATA_SIZE*MAX_SENSORS];//this implies that we are limited to 32 sensors at the moment
		uint8_t ids[UUID_WIDTH*MAX_SENSORS];
		void log(bool print = false){
			uint16_t index = 0;
			for (int i=0; i<_size;i++) {
				sensors[i]->log(print);
				uint8_t sensorDataSize = sensors[i]->getSize();
				//flipping MSB and LSB!
				for (int j=sensorDataSize-1; j>=0 ;j--){
					data[index++] = sensors[i]->data[j];	
				}
			}
			_count=0;
		}
		
		const uint8_t getDataSize(){	return sizeOfData;	};
		uint16_t _count;
};
#endif
