#include <stdint.h>
#include "Arduino.h"

#include "XBee.h"
#include "XBeePlus.h"

//#define DEBUG

#define SET_BIT(p,n) ((p) |= (1 << (n)))
#define CLR_BIT(p,n) ((p) &= (~(1) << (n)))

#define CTS_PORT PORTF
#define CTS_PIN 3

//CONSTRUCTOR
////////////////////////////////////////////////////////////////////////////
XBeePlus::XBeePlus()
{
	XBee _xbee = XBee();
	ZBRxResponse rx = ZBRxResponse();
	XBeeAddress64 addr64 = XBeeAddress64(0x0, 0xFFFF);
	ZBTxStatusResponse txStatus = ZBTxStatusResponse();
}

//PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////
void XBeePlus::begin(long baud)
{

	_baud = baud;
	PORTF.DIR |= PIN3_bm;
	PORTF.OUTCLR |= PIN3_bm;
	Serial4.begin(baud);
	_xbee.setSerial(Serial4);

	//begin Xbee Reset Add - Sept 2: for hard reset to Xbee from PortD.Pin1
	PORTD.PIN1CTRL = 0b00101111; //OPC = 101 -> WiredAND / ISC = 111 -> INPUT_DISABLE
	PORTD.OUT |= (0x02);  		 //PD1 off (open collector) Note* pin output is inverted
	PORTD.DIR |= (1<<1);         //pin 1 is set to output
	//end XBee Reset Add


}

void XBeePlus::hardReset()
{//add Xbee Hard Reset from PD1 - Sept. 2
	uint16_t pulse_ms = 100;
	Serial.println("Xbee hard reset");
	PORTD.OUT &= ~(0x02);  //PD1 on (pull down)
	delay(pulse_ms);
	PORTD.OUT |= (0x02);   //PD1 off (open collector)
}

void XBeePlus::reset(){
	Serial4.write("\r");
	Serial4.write("\r");
	Serial4.write("\r");
	Serial4.write("\r");
}



bool XBeePlus::CTS(){
	return !(PORTF.IN>>3&0b1);
}

bool XBeePlus::ready(){
	return CTS();
}

void XBeePlus::enable(){
	PORTF.OUTCLR|=PIN3_bm;
	Serial4.flush();
	//Serial4.begin(_baud);
}

void XBeePlus::disable(){
	PORTF.OUT|=PIN3_bm;
}

void XBeePlus::refresh()
{
	_xbee.readPacket();	
}

bool XBeePlus::available()
{
	return _xbee.getResponse().isAvailable();
}

void XBeePlus::meetCoordinator()
{
        //XBeeAddress64 tmp = 
	_xbee.getResponse().getZBRxResponse(rx);//.getRemoteAddress64();
        XBeeAddress64 tmp = rx.getRemoteAddress64();
	addr64.setMsb(tmp.getMsb());
        addr64.setLsb(tmp.getLsb());
}

uint8_t * XBeePlus::getData(){
	_xbee.getResponse().getZBRxResponse(rx);
	#ifdef DEBUG
	for(int i=0; i< rx.getDataLength();i++){
	  Serial.print(rx.getData(i),HEX);
          Serial.print(",");  
        }
	Serial.println();
	#endif
	uint8_t dataLength = rx.getDataLength();
	data[0] = rx.getData(dataLength-2);
	data[1] = rx.getData(dataLength-1);
	return &data[0];
}

#define ID_FRAME 0b0001
#define DATA_FRAME 0b0010

bool XBeePlus::sendIDs(uint8_t * arrayPointer, uint8_t arrayLength){
	addr64.setMsb(0x0);
	addr64.setLsb(0xFFFF);
	return sendApiframe(arrayPointer, arrayLength, ID_FRAME);	
}

bool XBeePlus::sendData(uint8_t * arrayPointer, uint8_t arrayLength){
	addr64.setMsb(0x0);
	addr64.setLsb(0xFFFF);
	return sendApiframe(arrayPointer, arrayLength, DATA_FRAME);
}

bool XBeePlus::sendApiframe(uint8_t *arrayPointer, uint8_t arrayLength, uint8_t frame){
	uint8_t arr[arrayLength+1];
        arr[0]=frame;
        for(uint8_t i=0; i<arrayLength; i++){
		
                arr[i+1]=arrayPointer[i];
        }

        return send(&arr[0], arrayLength+1,addr64.getMsb(), addr64.getLsb());
}

#define DEBUG
bool XBeePlus::send(uint8_t * arrayPointer, uint8_t arrayLength, uint32_t addr64_MSB, uint32_t addr64_LSB){
	addr64.setMsb(addr64_MSB);
	addr64.setLsb(addr64_LSB);
	
	ZBTxRequest zbTx = ZBTxRequest(addr64, arrayPointer, arrayLength);
	//zbTx.setOption(0xAAAA);	
	//Serial.print("Option: ");	
	//Serial.println(zbTx.getOption());
	_xbee.send(zbTx);	

  	if (_xbee.readPacket(5000)){
		uint8_t API_ID = _xbee.getResponse().getApiId();
    		if (API_ID == 89 || API_ID==139) {
      			_xbee.getResponse().getZBTxStatusResponse(txStatus);
	        	// get the delivery status, the fifth byte
      			if (txStatus.getDeliveryStatus() == SUCCESS) {
				#ifdef DEBUG
				Serial.println("success");
				Serial.println("---------------------------------------------------------------");
				#endif	
				// success.  time to celebrate			
				return true;
        			
        			
      			} else {
        			//the remote XBee did not receive our packet. is it powered on?
				#ifdef DEBUG				
				Serial.println("fail");
				#endif
				return false;        			

      			}
    		}
		else{
			Serial.print("Wrong API_ID: ");
			Serial.println(API_ID);
			return false;
		}
		
  	} else if (_xbee.getResponse().isError()) {
		#ifdef DEBUG
		Serial.print("Error reading packet.  Error code: ");  
		#endif
		Serial.println(_xbee.getResponse().getErrorCode());
		return false;
  	} else { // time elapsed with no answer
    	#ifdef DEBUG
		Serial.println("untimely response");
		#endif
		return false;
  	}	
}

XBeePlus xbee;

