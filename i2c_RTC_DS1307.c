/*
 * File name            : i2c_RTC_DS1307.c
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
#include "i2c.h"
#include "i2c_RTC_DS1307.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif
/****************RTC FUNCTIONS*****************/

/*Feed Current timestamp from gsm in to RTC*/
void feedTimeInRTC(void) {
    unsigned char day = 0x01; // Storing dummy day 'Monday'
    /* Convert time stamp to BCD format*/
    //setBCDdigit(0x0E,1); // (t) BCD indication for RTC Clock feed Action
    __delay_ms(500);
    #ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("feedTimeInRTC_IN\r\n");
    //********Debug log#end**************//
    #endif
    currentSeconds = decimal2BCD(currentSeconds); 
    currentMinutes = decimal2BCD(currentMinutes);
    currentHour = decimal2BCD(currentHour);
    currentDD = decimal2BCD(currentDD);
    currentMM = decimal2BCD(currentMM);
    currentYY = decimal2BCD(currentYY);
    rtc_i2cStart();          /*start condition */
    
    rtc_i2cWrite(0xD0);     /* slave address with write mode */
    rtc_i2cWrite(0x00);     /* address of seconds register written to the pointer */ 
    
    rtc_i2cWrite(currentSeconds);  /*time register values */
    rtc_i2cWrite(currentMinutes);
    rtc_i2cWrite(currentHour);
    
    rtc_i2cWrite(day);      /*date registers */
    rtc_i2cWrite(currentDD);
    rtc_i2cWrite(currentMM);
    rtc_i2cWrite(currentYY);
    
    rtc_i2cStop();          /*i2c stop condition */
    //setBCDdigit(0x0F,0); // Blank BCD Indication for Normal Condition
    #ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("feedTimeInRTC_OUT\r\n");
    //********Debug log#end**************//
    #endif
}

/* convert the decimal values to BCD using below function */
unsigned char decimal2BCD (unsigned char decimal) {
    unsigned char temp;
    temp = (unsigned char)((decimal/10) << 4);
    temp = temp | (decimal % 10);
    return temp;
}

/* Convert BCD to decimal */
unsigned char bcd2Decimal (unsigned char bcd) {
    unsigned char temp;
    temp = (bcd & 0x0F) + ((bcd & 0xF0)>>4)*10;
    return temp;
}

/* Fetch Current timestamp from RTC */
void fetchTimefromRTC(void) {
    unsigned char day = 0x01; // Storing dummy day 'Monday'
    //setBCDdigit(0x0E,0);  // (t.) BCD indication for RTC Clock fetch action
    __delay_ms(500);
    #ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("fetchTimefromRTC_IN\r\n");
    //********Debug log#end**************//
    #endif
    rtc_i2cStart();
	rtc_i2cWrite(0xD0);
	rtc_i2cWrite(0x00);
	rtc_i2cRestart();
	rtc_i2cWrite(0xD1);
	currentSeconds = rtc_i2cRead(1); /* Read the slave with ACK */
	currentMinutes = rtc_i2cRead(1);
	currentHour = rtc_i2cRead(1);
    day = rtc_i2cRead(1);
	currentDD = rtc_i2cRead(1);
	currentMM = rtc_i2cRead(1);
	currentYY = rtc_i2cRead(0);
    rtc_i2cStop();
    
    /* Convert Timestamp to decimal*/
    currentSeconds = bcd2Decimal(currentSeconds); 
    currentMinutes = bcd2Decimal(currentMinutes);
    currentHour = bcd2Decimal(currentHour);
    currentDD = bcd2Decimal(currentDD);
    currentMM = bcd2Decimal(currentMM);
    currentYY = bcd2Decimal(currentYY);
    //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
    #ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("fetchTimefromRTC_OUT\r\n");
    //********Debug log#end**************//
    #endif
}

/**********************************************/

