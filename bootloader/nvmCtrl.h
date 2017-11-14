/*
 * nvmCom.h
 *
 * Created: 10/31/2017 3:50:37 PM
 *  Author: anima
 */ 

#include "sam.h"

#define APP_START_ADDRESS 0x00001000
#define NVM_READY_STATE NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY

class Flasher {
public:
	Flasher();
	void inputStream(void);
	void eraseRowNum(uint16_t rowNum);
	void writePage(void);
	
private:
	uint8_t com;
	uint8_t tempC;
	bool comApproved;
	uint16_t numRows;
	uint16_t curRP;
	uint16_t numPages;
	uint16_t writePages;
	
};