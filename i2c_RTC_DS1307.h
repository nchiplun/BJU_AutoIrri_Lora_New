/*
 * File name            : i2c_RTC_DS1307.h
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : RTC_DS1307 functions header file
 */

#ifndef I2C_RTC_DS1307_H
#define	I2C_RTC_DS1307_H

//-----------[ Functions' Prototypes ]--------------

//---[ RTC Routines ]---
void feedTimeInRTC(void);
void fetchTimefromRTC(void);
unsigned char decimal2BCD (unsigned char);
unsigned char bcd2Decimal (unsigned char bcd);

#endif
 /* RTC_DS1307_H */