#include "UART.h"

UART::UART(Sercom* _s, uint8_t _serAlt, uint8_t _prt, uint8_t _pinRx, uint8_t _pinTx, uint8_t _padRx, uint8_t _padTx, voidFuncPtr _func) {
	sercom = _s;
	serAlt = _serAlt;
	prt = _prt;
	pinRx = _pinRx;
	pinTx = _pinTx;
	padRx = _padRx;
	padTx = _padTx;
	
	rxBufHead = 0;
	rxBufTail = 0;
	txBufHead = 0;
	txBufTail = 0;
	
	rxFunc = _func;
}

void UART::begin(uint32_t baudRate) {
	PORT->Group[prt].WRCONFIG.reg = (uint32_t)(PORT_WRCONFIG_WRPINCFG | PORT_WRCONFIG_HWSEL |
		PORT_WRCONFIG_WRPMUX | PORT_WRCONFIG_PMUXEN | 1 << (pinRx - 16) | 1 << (pinTx - 16) |
		((2 + serAlt) << PORT_WRCONFIG_PMUX_Pos));
	
	uint8_t clockId = 0x19;
	IRQn_Type IdNvic = SERCOM5_IRQn;
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM5 ;
	//Interrupt
	NVIC_EnableIRQ(IdNvic);
	NVIC_SetPriority(IdNvic, (1<<__NVIC_PRIO_BITS) - 1);
	//Generic Clock
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(clockId) | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	//reset UART
	while(sercom->USART.SYNCBUSY.bit.ENABLE);
	sercom->USART.CTRLA.bit.ENABLE = 0;
	sercom->USART.CTRLA.bit.SWRST = 1;
	while(sercom->USART.CTRLA.bit.SWRST || sercom->USART.SYNCBUSY.bit.SWRST);
	//Interrupt 2
	sercom->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC | SERCOM_USART_INTENSET_TXC;
	//baudrate
	sercom->USART.BAUD.reg = 63019;
	//CTRL and Frame and Pads
	sercom->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE(0x1) | SERCOM_USART_CTRLA_SAMPR(0x0) |
		SERCOM_USART_CTRLA_FORM(0) | 1 << SERCOM_USART_CTRLA_DORD_Pos | SERCOM_USART_CTRLA_TXPO((padTx/2)) | 
		SERCOM_USART_CTRLA_RXPO(padRx); //INT CLK, 16 samp & arithmetic baud, no parity, LSB, Pads
	sercom->USART.CTRLB.reg = SERCOM_USART_CTRLB_CHSIZE(0) | 0 << SERCOM_USART_CTRLB_SBMODE_Pos |
		0 << SERCOM_USART_CTRLB_PMODE_Pos | SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN; 
		//8 bit, 1 stop bit, even parity NO, enable interfaces
	//enable UART
	sercom->USART.CTRLA.bit.ENABLE = 1;
	while(sercom->USART.SYNCBUSY.bit.ENABLE);
	
	
}

uint8_t UART::availableBytes(void) {
	if (rxBufTail < rxBufHead) {
		return (256 - rxBufHead + rxBufTail);
	} else {
		return (rxBufTail - rxBufHead);
	}
}

void UART::irqHandler(void) {
	if (sercom->USART.INTFLAG.bit.RXC) {
		rxBuf[rxBufTail++] = sercom->USART.DATA.bit.DATA;
		rxFunc();
	} else if(sercom->USART.INTFLAG.bit.TXC) {
		send();
	}
}

void UART::send(void) {
	if (txBufHead != txBufTail) {
		while (!sercom->USART.INTFLAG.bit.DRE);
		sercom->USART.DATA.reg = txBuf[txBufHead++];
	} else {
		sercom->USART.INTFLAG.bit.TXC = 1;
	}
}

uint8_t UART::read(void) {
	if (rxBufHead != rxBufTail) {
		return rxBuf[rxBufHead++];
	} else {
		return 0;
	}
}

uint16_t UART::write(const uint8_t data) {
	txBuf[txBufTail] = data;
	if (txBufTail == txBufHead) {
		txBufTail++;
		send();
	} else {
		txBufTail++;
	}
	return 1;
}




































