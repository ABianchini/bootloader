/*
 * CRC_16.h
 *
 * Created: 11/1/2017 10:32:47 AM
 *  Author: anima
 */ 
#pragma once

#define POLY 0x7DF2

uint16_t crcExtractData(uint32_t inp) {
	return (inp >> 16);
}

bool crcVerifyData(uint32_t data) {
	return (data % POLY == 0);
}