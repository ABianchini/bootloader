/*
 * nvmCom.cpp
 *
 * Created: 10/31/2017 3:50:48 PM
 *  Author: anima
 */ 

#include "nvmCtrl.h"
#include "UART.h"
#include "CRC-16.h"

extern UART Serial;

#define NVM_MEMORY        ((volatile uint16_t *)FLASH_ADDR)

Flasher::Flasher() {
	NVMCTRL->PARAM.bit.NVMP = numPages;
	numRows = numPages / 4;
	com = 0x00;
	comApproved = false;
	tempC = 0x00;
}

void Flasher::inputStream(void) {
	if (com) {
		switch (com) {
			case 0xEE:
				if (Serial.read() == 0x55) {
					eraseRowNum(curRP);
					Serial.write(curRP>>8);
					Serial.write(curRP&0xFF);
					curRP++;
				} else {
					com = 0x00;
					curRP = 0;
				}
				break;
			case 0xBB:
				com = 0x00;
				break;
			case 0xAA:
				if (comApproved) {
					if (Serial.availableBytes() == 130) {
						writePage();
					}
				} else {
					if (Serial.read() == 0x55) {
						comApproved = true;
					} else  {
						com = 0x00;
					}
				}
				break;
		}
		
	} else {
		if (tempC) {
			if (Serial.availableBytes() == 2) {
				com = tempC;
				tempC = 0x00;
				writePages = Serial.read();
				writePages <<= 8;
				writePages |= Serial.read();
				Serial.write(com);
				Serial.write(writePages >> 8);
				Serial.write(writePages & 0xFF);
				curRP = 0;
			}
		} else {
			tempC = Serial.read();
			switch (tempC) {
			case 0xBB:
				com = tempC;
				tempC = 0x00;
				Serial.write(com);
				break;
			case 0xEE:
				com = tempC;
				tempC = 0x00;
				Serial.write(com);
				Serial.write(numRows >> 8);
				Serial.write(numRows & 0xFF);
				curRP = 0;
			}
		}
	}
}

void Flasher::eraseRowNum(uint16_t rowNum) {
	uint32_t destAddr = rowNum * 256;
	if (destAddr >= APP_START_ADDRESS) {
		
		while (!(NVM_READY_STATE));
		NVMCTRL->STATUS.reg &= ~NVMCTRL_STATUS_MASK;
		
		NVMCTRL->ADDR.reg = (uintptr_t)&NVM_MEMORY[destAddr / 4];
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
		while(NVM_READY_STATE);
	}
}

void Flasher::writePage(void) {
	curRP = Serial.read();
	curRP <<= 8;
	curRP |= Serial.read();
	uint32_t destAddr = curRP * 64;
	if (destAddr >= APP_START_ADDRESS) {
		while (!(NVM_READY_STATE));
		NVMCTRL->STATUS.reg &= ~NVMCTRL_STATUS_MASK;
		
		destAddr /= 2;
		uint16_t shortT = 0x0000;
		uint32_t longT = 0x00000000;
		for (uint8_t i = 0; i < 128; i += 4) {
			longT = 0x00000000;
			for (uint8_t x = 0; x < 4; x++) {
				longT |= Serial.read();
				longT <<= 8;
			}
			if (!crcVerifyData(longT)) {
				goto failCRC;
			}
			shortT = crcExtractData(longT);
			NVM_MEMORY[destAddr++] = shortT;
		}
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_WP | NVMCTRL_CTRLA_CMDEX_KEY;
		while (NVM_READY_STATE);
	}
	if (curRP == (writePages - 1)) Serial.write(0x55);
	
	Serial.write(0x55);
	return;
	
	failCRC:
		Serial.write(0x11);
}