/*
 * bootloader.cpp
 *
 * Created: 10/31/2017 3:32:13 PM
 * Author : anima
 */ 


#include "sam.h"
#include "UART.h"
#include "nvmCtrl.h"

#define APP_START_ADDR APP_START_ADDRESS //CHANGE!!!!!!!

void rxFunc(void);
void bootloaderCheck(void) {
	uint32_t appStartAddress = *(uint32_t *)(APP_START_ADDRESS + 4);
	if ((PORT->Group[0].IN.reg & (1<<28)) != 0) {
		if (appStartAddress == 0xFFFFFFFF) {
			return;
			} else {
			/* Rebase the Stack Pointer */
			__set_MSP(*(uint32_t *) APP_START_ADDRESS);

			/* Rebase the vector table base address */
			SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

			/* Jump to application Reset Handler in the application */
			asm("bx %0"::"r"(appStartAddress));
		}
		} else {
		return;
	}
}

UART Serial(SERCOM5,1,1,23,22,3,2,rxFunc);
Flasher nvm;

int main(void)
{
	SystemCoreClock = 1000000;
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val;
	PM->APBAMASK.reg |= PM_APBAMASK_GCLK;
	//Setup bootloader pin
	PORT->Group[0].DIRSET.reg = (uint32_t)(1<<28);
	PORT->Group[0].PINCFG[28].reg = PORT_PINCFG_PULLEN | PORT_PINCFG_INEN;
	PORT->Group[0].OUTSET.reg = (uint32_t)(1<<28);
	
	bootloaderCheck();
	
	//Initialize the SAM system
	SystemInit();
	Serial.begin(115200);
	
	while (1) {
		
	}
}

void rxFunc(void) {
	nvm.inputStream();
}

void SERCOM5_Handler() {
	Serial.irqHandler();
}


