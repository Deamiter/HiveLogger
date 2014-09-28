#ifndef XBeePlus_h
#define XBeePlus_h

#include <stdint.h>
#include "Arduino.h"
#include "XBee.h"

//asume data from coordinator will always be 2 bytes long
#define RX_DATA_LENGTH 2

class XBeePlus 
{
	public:
		XBeePlus();
		void begin(long);
		void reset();
		void disable();
		void enable();
		void refresh();
		bool available();
		uint8_t * getData();
		bool send(uint8_t *, uint8_t,uint32_t addr64_MSB=0x0, uint32_t addr_LSB=0xFF);
		bool sendIDs(uint8_t *, uint8_t);
		bool sendData(uint8_t *, uint8_t);
		bool sendApiframe(uint8_t *, uint8_t, uint8_t);
		bool ready();
		bool CTS();
		void meetCoordinator();
		void hardReset();
	private:
		ZBTxStatusResponse txStatus;
		XBeeAddress64 addr64;
		XBee _xbee;
		ZBRxResponse rx;
		uint8_t data[RX_DATA_LENGTH];
		uint16_t _baud;
};

extern XBeePlus xbee;
#endif
