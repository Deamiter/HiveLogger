#include <Sensorhub.h>

TemperatureSensor temp;

#define NUM_SENSORS 3
Sensor *sensorArray[] = {&temp, &temp, &temp};


void setup(){
  Serial.begin(57600);
  Serial.println("STARTUP");
}

void loop(){
  for(int i=0; i<NUM_SENSORS; i++){
    sensorArray[i]->getSensor();
    sensorArray[i]->getEvent();
    Serial.print("This sensor receives ");
    Serial.print(sensorArray[i]->getSize());
    Serial.println(" bytes of raw data");
    Serial.print("Data: ");
    for(uint8_t j=0; j<sensorArray[i]->getSize(); j++){
      Serial.print("[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.print(sensorArray[i]->raw[j]);
      Serial.print(", ");
    }
    Serial.println();
  }
  delay(1000);
}
