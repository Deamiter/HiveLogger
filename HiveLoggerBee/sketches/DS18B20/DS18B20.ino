#include <Clock.h>
#include <Onboard.h>
#include <XBeePlus.h>
#include <Wire.h>  //we'll be depending on the core's Wire library
#include <Sensorhub.h>
#include <Bee.h>
#include <WeatherPlug.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define DSB1820B_UUID 0x0018
#define DSB1820B_LENGTH_OF_DATA 2
#define DSB1820B_SCALE 100
#define DSB1820B_SHIFT 0

//Create Sensorhub Sensor from DallasTemp library
class DSB1820B: public Sensor
{
        public:
                OneWire oneWire;
                DallasTemperature dsb = DallasTemperature(&oneWire);
                
                DSB1820B(uint8_t samplePeriod=1):Sensor(DSB1820B_UUID, DSB1820B_LENGTH_OF_DATA, DSB1820B_SCALE, DSB1820B_SHIFT, true, samplePeriod){};
                String getName(){ return "Temperature Probe"; }
                String getUnits(){ return "C"; }
                void init(){
                  weatherPlug.init();
                }
                void getData(){
                  
                  weatherPlug.i2cChannel(2);
                  weatherPlug.disableI2C();
                  delay(5);
                  dsb.begin();
                  dsb.requestTemperatures();
                  float sample = dsb.getTempCByIndex(4);
                  uint16_t tmp = (sample + DSB1820B_SHIFT) * DSB1820B_SCALE;
                  data[1]=tmp>>8;
                  data[0]=tmp;
                  weatherPlug.enableI2C();
                }

};

#define NUM_SAMPLES 32
DateTime date = DateTime(__DATE__, __TIME__);

OnboardTemperature onboardTemp;
BatteryGauge batteryGauge;
DSB1820B dsb;

#define NUM_SENSORS 3
Sensor * sensor[] = {&onboardTemp, &batteryGauge, &dsb};
Sensorhub sensorhub(sensor,NUM_SENSORS);

#define DEBUG
//#define XBEE_ENABLE

void setup(){

  Serial.begin(57600);
  delay(1000);
  
  clock.begin(date);
  configureSleep();
  sensorhub.init();
  
  #ifdef DEBUG
  Serial.println("IDs:");
  Serial.print("[");
  for(int i=0; i<UUID_WIDTH*NUM_SENSORS;i++){
    Serial.print(sensorhub.ids[i]);
    Serial.print(", ");
  }
  Serial.print("]");
  Serial.println();
  
  Serial.println("starting");
  #endif
  
  #ifdef XBEE_ENABLE
  xbee.begin(9600);
  xbee.sendIDs(&sensorhub.ids[0], UUID_WIDTH*NUM_SENSORS);
  xbee.refresh();
  while(!xbee.available()){
    xbee.refresh();
  }
  xbee.meetCoordinator();
  #endif
  

}

void loop(){
  clock.print();
  for(int i=0;i<NUM_SAMPLES;i++) sensorhub.sample();
 
 
  #ifdef DEBUG 
  sensorhub.log(true);
  #else
  sensorhub.log();
  #endif
  
  #ifdef XBEE_ENABLE
  xbee.sendData(&sensorhub.data[0], sensorhub.getDataSize());
  #endif
  
  clock.setAlarm1Delta(0, 15);
  
  sleep();
}



