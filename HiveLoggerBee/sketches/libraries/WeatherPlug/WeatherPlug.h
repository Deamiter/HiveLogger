#ifndef WeatherPlug_h
#define WeatherPlug_h

#define ADS7828 0b1001000
#define PCA9540 0b1110000
#define SHT2x 0b1000000

#define NUM_WIND_MAPPINGS 16
#define NUM_WIND_CUTOFFS NUM_WIND_MAPPINGS-1

#include <utility/Adafruit_BMP085.h>

class WeatherPlug
{
	public:
                ////////////////////////////////////////////////////////////////////////
		//	shared resources by drivers
                ////////////////////////////////////////////////////////////////////////
		void init();
		void wake();
		void sleep();
		void i2cChannel(uint8_t channel);
		void enableI2C();
		void disableI2C();
		bool initialized=false;
		bool isAwake=false;
		uint16_t _getADC(uint16_t channel);	
		//BMP085 temp & pressure
		Adafruit_BMP085 bmp;

		//wind direction
		static uint16_t windDirCutoffs[NUM_WIND_CUTOFFS];
		static float windDirMappings[NUM_WIND_MAPPINGS];
		float mapWindDirection(uint16_t val);
		float getWindDirection();
		//wind speed
		volatile uint32_t windCount = 0;
		uint8_t W_sec;
		uint8_t W_min;
		uint8_t W_hour;
		void startWind();
		float getWindCoeff();
		float getWindSpeed();
		//rainfall
		float getRainfall();
		volatile uint32_t rainCount = 0;

		//SHT2x
		float SHT_getTemp();
		float SHT_getRH();
};

extern WeatherPlug weatherPlug;

#define BMP_TEMP_UUID 0x0010
#define BMP_TEMP_LENGTH_OF_DATA 2
#define BMP_TEMP_SCALE 100
#define BMP_TEMP_SHIFT 50

class BMP_Temperature: public Sensor
{
        public:
                BMP_Temperature(uint8_t samplePeriod=1):Sensor(BMP_TEMP_UUID, BMP_TEMP_LENGTH_OF_DATA, BMP_TEMP_SCALE, BMP_TEMP_SHIFT, false,samplePeriod){};
                String getName();
                String getUnits();
                void init();
                void getData();
};

#define BMP_PRESS_UUID 0x0011
#define BMP_PRESS_LENGTH_OF_DATA 3
#define BMP_PRESS_SCALE 1
#define BMP_PRESS_SHIFT 0

class BMP_Pressure: public Sensor
{
        public:
                BMP_Pressure(uint8_t samplePeriod=1):Sensor(BMP_PRESS_UUID, BMP_PRESS_LENGTH_OF_DATA, BMP_PRESS_SCALE, BMP_PRESS_SHIFT, false, samplePeriod){};
                String getName();
                String getUnits();
                void init();
                void getData();
};

#define WIND_DIR_UUID 0x0012
#define WIND_DIR_LENGTH_OF_DATA 2
#define WIND_DIR_SCALE 100
#define WIND_DIR_SHIFT 0

class WindDirection: public Sensor
{
        public:
		WindDirection(uint8_t samplePeriod=1):Sensor(WIND_DIR_UUID, WIND_DIR_LENGTH_OF_DATA, WIND_DIR_SCALE, WIND_DIR_SHIFT, false, samplePeriod){};
	        String getName();
                String getUnits();
		void init();
                void getData();      
};

#define WIND_SPEED_UUID 0x0013
#define WIND_SPEED_LENGTH_OF_DATA 2
#define WIND_SPEED_SCALE 1000
#define WIND_SPEED_SHIFT 0

class WindSpeed: public Sensor
{
	public:
		WindSpeed(uint8_t samplePeriod=1):Sensor(WIND_SPEED_UUID, WIND_SPEED_LENGTH_OF_DATA, WIND_SPEED_SCALE, WIND_SPEED_SHIFT, true, samplePeriod){};
	        String getName();
                String getUnits();
		void init();
                void getData();      
};
#endif

#define RAINFALL_UUID 0x0014
#define RAINFALL_LENGTH_OF_DATA 2
#define RAINFALL_SCALE 1000
#define RAINFALL_SHIFT 0

class Rainfall: public Sensor
{
        public:
                Rainfall(uint8_t samplePeriod=1):Sensor(RAINFALL_UUID, RAINFALL_LENGTH_OF_DATA, RAINFALL_SCALE, RAINFALL_SHIFT, true, samplePeriod){};
                String getName();
                String getUnits();
                void init();
                void getData();
};

#define SHT2x_TEMP_UUID 0x0015
#define SHT2x_TEMP_LENGTH_OF_DATA 2
#define SHT2x_TEMP_SCALE 100
#define SHT2x_TEMP_SHIFT 50

class SHT2x_temp: public Sensor
{
        public:
                SHT2x_temp(uint8_t samplePeriod=1):Sensor(SHT2x_TEMP_UUID, SHT2x_TEMP_LENGTH_OF_DATA, SHT2x_TEMP_SCALE, SHT2x_TEMP_SHIFT, true, samplePeriod){};
                String getName();
                String getUnits();
                void init();
                void getData();
};

#define SHT2x_RH_UUID 0x0016
#define SHT2x_RH_LENGTH_OF_DATA 2
#define SHT2x_RH_SCALE 100
#define SHT2x_RH_SHIFT 0

class SHT2x_RH: public Sensor
{
        public:
                SHT2x_RH(uint8_t samplePeriod=1):Sensor(SHT2x_RH_UUID, SHT2x_RH_LENGTH_OF_DATA, SHT2x_RH_SCALE, SHT2x_RH_SHIFT, true, samplePeriod){};
                String getName();
                String getUnits();
                void init();
                void getData();
};

