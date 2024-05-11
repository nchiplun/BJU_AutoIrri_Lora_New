/*
 * File name            : RTC_DS1307.h
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : RTC_DS1307 functions header file
 */

#ifndef I2C_H
#define	I2C_H

//-----------[ Functions' Prototypes ]--------------
 
//---[ RTC_I2C Routines ]---
void rtc_i2cStart(void);
void rtc_i2cRestart(void);
void rtc_i2cStop(void);
void rtc_i2cWait(void);
void rtc_i2cWrite(unsigned char);
unsigned char rtc_i2cRead(_Bool);


//---[ LCD_I2C Routines ]---
void lcd_i2cWait(void);
void lcd_i2cStart(void);
void lcd_i2cRestart(void);
void lcd_i2cStop(void);
void lcd_i2cWrite(unsigned char);
void lcd_i2cWriteByteSingleReg(unsigned char device, unsigned char info);

#endif
 /* I2C_H */