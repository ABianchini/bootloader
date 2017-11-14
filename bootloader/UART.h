#pragma once

#include <math.h>
#include <stdint.h>
#include "sam.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

typedef void(*voidFuncPtr)(void);


class UART {
public:
	
	UART(Sercom* _s, uint8_t serAlt, uint8_t _prt, uint8_t _pinRx, uint8_t _pinTx, uint8_t _padRx, uint8_t _padTx, voidFuncPtr _func = UART::emptyFunc);
	void begin(uint32_t baudRate);
	uint8_t read(void);
	uint8_t availableBytes(void);
	void irqHandler(void);
	
	uint16_t write(const uint8_t data);
	
	
	
private:
	Sercom* sercom;
	uint8_t serAlt;
	uint8_t prt;
	uint8_t pinRx;
	uint8_t pinTx;
	uint8_t padRx;
	uint8_t padTx;
	
	uint8_t rxBuf[256];
	uint8_t txBuf[256];
	uint8_t rxBufHead;
	uint8_t rxBufTail;
	uint8_t txBufHead;
	uint8_t txBufTail;
	
	voidFuncPtr rxFunc;
	static void emptyFunc(void) {}
	void send(void);
	
};