/*
 * File name            : i2c.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : Main source file
 */


#include "congfigBits.h"
#include "variableDefinitions.h"
#include "controllerActions.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif

#define I2C_WRITE   0b11111110
#define I2C_READ    0b00000001

/****************RTC_I2C-Library*******************/

void rtc_i2cStart(void) {
	SSP2CON2bits.SEN = 1;
	while (SSP2CON2bits.SEN == SET);
	//SEN =1 initiate the Start Condition on SDA and SCL Pins
	//Automatically Cleared by Hardware
	// 0 for Idle State
}

void rtc_i2cRestart(void) {
	SSP2CON2bits.RSEN = 1;
	while (SSP2CON2bits.RSEN == SET);
	//RSEN = 1 initiate the Restart Condition
	//Automatically Cleared by Hardware
}

void rtc_i2cStop(void) {
	SSP2CON2bits.PEN = 1;
	while (SSP2CON2bits.PEN == SET);
}

void rtc_i2cWait(void) {
    while ((SSP2CON2 & 0x1F) | (SSP2STATbits.R_NOT_W));
    // Wait condition until I2C bus is Idle.
}

void rtc_i2cWrite(unsigned char data) {
	SSP2BUF = data;    /* Move data to SSPBUF */
    while (SSP2STATbits.BF);       /* wait till complete data is sent from buffer */
    rtc_i2cWait();       /* wait for any pending transfer */
}

unsigned char rtc_i2cRead(_Bool ACK) {
	unsigned char temp;
    SSP2CON2bits.RCEN = 1;
    /* Enable data reception */
    while (SSP2STATbits.BF == CLEAR);      /* wait for buffer full */
    temp = SSP2BUF;   /* Read serial buffer and store in temp register */
    rtc_i2cWait();       /* wait to check any pending transfer */
    if (ACK)
        SSP2CON2bits.ACKDT=0;               //send acknowledge
    else
        SSP2CON2bits.ACKDT=1;				//Do not  acknowledge
	SSP2CON2bits.ACKEN=1;
	while (SSP2CON2bits.ACKEN == SET)
        ;
    return temp;     /* Return the read data from bus */
}
/**********************************************/

/****************LCD_I2C-Library*******************/

void lcd_i2cWait(void) {
    while ((SSP1CON2 & 0x1F) | (SSP1STATbits.R_NOT_W));
    // Wait condition until I2C bus is Idle.
}

void lcd_i2cStart(void) {
	SSP1CON2bits.SEN = 1;
	while (SSP1CON2bits.SEN == SET);
	//SEN =1 initiate the Start Condition on SDA and SCL Pins
	//Automatically Cleared by Hardware
	// 0 for Idle State
}

void lcd_i2cRestart(void) {
	SSP1CON2bits.RSEN = 1;
	while (SSP1CON2bits.RSEN == SET);
	//RSEN = 1 initiate the Restart Condition
	//Automatically Cleared by Hardware
}

void lcd_i2cStop(void) {
	SSP1CON2bits.PEN = 1;
	while (SSP1CON2bits.PEN == SET);
}

void lcd_i2cWrite(unsigned char data) {
	SSP1BUF = data;                 /* Move data to SSPBUF */
    while (SSP1STATbits.BF);        /* wait till complete data is sent from buffer */
    lcd_i2cWait();                  /* wait for any pending transfer */
}

void lcd_i2cWriteByteSingleReg(unsigned char device, unsigned char info)
{
    lcd_i2cWait();
    lcd_i2cStart();
    lcd_i2cWrite(device & I2C_WRITE);
    lcd_i2cWait();
    lcd_i2cWrite(info);
    lcd_i2cStop();
}
/**********************************************/