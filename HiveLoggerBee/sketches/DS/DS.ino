// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

#include <OneWire.h>

#include <DallasTemperature.h>
OneWire oneWire;
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

void setup(){
// start serial port
  Serial.begin(57600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  
  sensors.begin();
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
}

void loop(){
// call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  float tempC = sensors.getTempCByIndex(0);
  Serial.print("Temp C: ");
  Serial.println(tempC);
}
