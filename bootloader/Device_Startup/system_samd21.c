/**
 * \file
 *
 * \brief Low-level initialization functions called upon chip startup.
 *
 * Copyright (c) 2016 Atmel Corporation,
 *                    a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#include "samd21.h"

/**
 * Initial system clock frequency. The System RC Oscillator (RCSYS) provides
 *  the source for the main clock at chip startup.
 */
#define __SYSTEM_CLOCK    (1000000)
#define VARIANT_MCK		(48000000ul)
#define VARIANT_MAINOSC (32768ul)
//clock generator constants
#define GENERIC_CLOCK_GENERATOR_MAIN      (0u)
#define GENERIC_CLOCK_GENERATOR_XOSC32K   (1u)
#define GENERIC_CLOCK_GENERATOR_OSC32K    (1u)
#define GENERIC_CLOCK_GENERATOR_OSCULP32K (2u) /* Initialized at reset for WDT */
#define GENERIC_CLOCK_GENERATOR_OSC8M     (3u)
//clock multiplexer constant
#define GENERIC_CLOCK_MULTIPLEXER_DFLL48M (0u)

uint32_t SystemCoreClock = __SYSTEM_CLOCK;/*!< System Clock Frequency (Core Clock)*/

/**
 * Initialize the system
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
void SystemInit(void)
{
	//1 - enable XOSC32K clock (external 32kHz osc)
	SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(0x6u) |
	SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_EN32K;
	SYSCTRL->XOSC32K.bit.ENABLE = 1;
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSC32KRDY) == 0);
	//reset GCLK
	GCLK->CTRL.reg = GCLK_CTRL_SWRST;
	while ((GCLK->CTRL.reg & GCLK_CTRL_SWRST) && (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY));
	
	//2 - put XOSC32K as source for Generic Clock Generator 1
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(GENERIC_CLOCK_GENERATOR_XOSC32K);
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	//write Generic Clock Generator 1 config
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(GENERIC_CLOCK_GENERATOR_XOSC32K) |
	GCLK_GENCTRL_SRC_XOSC32K | GCLK_GENCTRL_GENEN;
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	
	//3 - Put Generic Clock Generator 1 as source of Generic Clock Multiplexer 0
	//		(DFLL48M reference)
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GENERIC_CLOCK_MULTIPLEXER_DFLL48M)|
	GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	
	//4 - Enable DFLL48M clock
	//remove on demand
	SYSCTRL->DFLLCTRL.bit.ONDEMAND = 0;
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);
	//set coarse and fine step, and XOSC32K as reference
	SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31) |
	SYSCTRL_DFLLMUL_FSTEP(511) | SYSCTRL_DFLLMUL_MUL((VARIANT_MCK/VARIANT_MAINOSC));
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);
	//full config to DFLL control reg: closed loop, disable quick lock
	SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_MODE |
	SYSCTRL_DFLLCTRL_WAITLOCK | SYSCTRL_DFLLCTRL_QLDIS;
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);
	//enable DFLL
	SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_ENABLE;
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKC) == 0 ||
	(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLLCKF) == 0);
	while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0);
	
	//5 - Switch Generic Clock Generator 0 to DFLL48M, CPU runs at 48MHz
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(GENERIC_CLOCK_GENERATOR_MAIN);
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	//Generic Clock Generator 0 config
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(GENERIC_CLOCK_GENERATOR_MAIN) |
	GCLK_GENCTRL_SRC_DFLL48M | GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;
	while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	
	//6 - Change OSC8M from 1MHz to 8MHz
	SYSCTRL->OSC8M.bit.PRESC = SYSCTRL_OSC8M_PRESC_1_Val;
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
	
	//7 - Generic Clock Generator 3 to OSC8M
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(GENERIC_CLOCK_GENERATOR_OSC8M);
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(GENERIC_CLOCK_GENERATOR_OSC8M) |
	GCLK_GENCTRL_SRC_OSC8M | GCLK_GENCTRL_GENEN;
	while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);
	//CPU and Advanced Peripheral Bus Clocks (APB)
	PM->CPUSEL.reg = PM_CPUSEL_CPUDIV_DIV1;
	PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1;
	PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1;
	PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1;
	SystemCoreClock = VARIANT_MCK;
	
	//9 - Disable automatic VM write operations
	NVMCTRL->CTRLB.bit.MANW = 1;
}

/**
 * Update SystemCoreClock variable
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
void SystemCoreClockUpdate(void)
{
	// Not implemented
	SystemCoreClock = __SYSTEM_CLOCK;
	return;
}
