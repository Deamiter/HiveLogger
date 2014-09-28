#define GSM Serial1

#define VREG_EN 6

void setup(){
  //TURN THINGS ON!
  pinMode(VREG_EN,OUTPUT);
  digitalWrite(VREG_EN,HIGH);
  
  pinMode(9,OUTPUT);
  digitalWrite(9,LOW);
  //PORTC.DIR|=0b10000;
  //PORTC.OUT|=0b10000;
    

  GSM.begin(9600);
  Serial.begin(57600);
  Serial.println("Oh hey!");

}

uint32_t count=0;
void loop(){
  GSM.println(count++,HEX);
  Serial.println(count++,HEX);
  delay(100);
}
