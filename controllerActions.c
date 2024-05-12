/*
 * File name            : controllerActions.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : Controller general functions
 */

#include "congfigBits.h"
#include "variableDefinitions.h"
#include "ADC.h"
#include "controllerActions.h"
#include "eeprom.h"
#include "gsm.h"
#include "lora.h"
#include "i2c.h"
#include "i2c_RTC_DS1307.h"
#include "i2c_LCD_PCF8574.h"
#ifdef Encryption_ON_H
#include "dataEncryption.h"
#endif
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif
/************************general purpose functions_start*******************************/

/*************************************************************************************************************************

This function is called to copy string from source to destination
The purpose of this function is to copy string till it finds first \n character in source.

 **************************************************************************************************************************/

char *strcpyCustom(char *restrict dest, const char *restrict src) {
    const char *s = src;
    char *d = dest;
    while ((*d++ = *s++))
        if (*s == '\n' || *s == '\r' || *s == '\0')
            break;
    return dest;
}

/*************************************************************************************************************************

This function is called to check if character is Numeric.
The purpose of this function is to check if the given character is in range of numeric ascii code

 **************************************************************************************************************************/
_Bool isNumber(unsigned char character) {
    if (character >= 48 && character <= 57) {
        return true;
    } else
        return false;
}

#ifdef Encryption_ON_H

/*************************************************************************************************************************

This function is called to check if string is Base64 encoded
The purpose of this function is to check if string has space or = or multiple of 4.

 **************************************************************************************************************************/
_Bool isBase64String(unsigned char * string) {
    //unsigned int stringLength;
    unsigned char * s = string;
    while (*s++ != '\0') {
        if (*s == space) {
            return false;
        }
    }
    return true;
}
#endif

/************************Get Current Clock Time From GSM#Start************************************/

/*************************************************************************************************************************

This function is called to get current local time
The purpose of this function is to receive local time stamp from GSM module

 **************************************************************************************************************************/
void getDateFromGSM(void) {
    unsigned char index = 0;
    timer3Count = 30; // 30 sec window
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("getDateFromGSM_IN\r\n");
    //********Debug log#end**************//
#endif
    controllerCommandExecuted = false;
    msgIndex = CLEAR;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    //setBCDdigit(0x0B, 1); // (]) BCD indication for getDateFromGSM action
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT+CCLK?\r\n"); // To get local time stamp  +CCLK: "18/05/26,12:00:06+22"   ok
        if (!controllerCommandExecuted) {
            __delay_ms(2500);
            __delay_ms(2500);
        }
    }
    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
    PIR5bits.TMR3IF = SET; //Stop timer thread
    // ADD indication if infinite
    tensDigit = CLEAR;
    unitsDigit = CLEAR;
    currentYY = CLEAR;
    currentMM = CLEAR;
    currentDD = CLEAR;
    currentHour = CLEAR;
    currentMinutes = CLEAR;
    currentSeconds = CLEAR;


    // To check no garbage value received for date time command
    for (index = 8; index < 22; index += 2) {
        if (isNumber(gsmResponse[index]) && isNumber(gsmResponse[index + 1])) {
            controllerCommandExecuted = true;
        } else {
            controllerCommandExecuted = false;
        }
    }

    if (!controllerCommandExecuted) {
        //RESET();
        // set indication of reset due to incorrect time stamp
    }
    else {

        tensDigit = gsmResponse[8] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[9] - 48;
        currentYY = tensDigit + unitsDigit; // Store year in decimal

        tensDigit = gsmResponse[11] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[12] - 48;
        currentMM = tensDigit + unitsDigit; // Store month in decimal

        tensDigit = gsmResponse[14] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[15] - 48;
        currentDD = tensDigit + unitsDigit; // Store day in decimal

        tensDigit = gsmResponse[17] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[18] - 48;
        currentHour = tensDigit + unitsDigit; // Store hour in decimal

        tensDigit = gsmResponse[20] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[21] - 48;
        currentMinutes = tensDigit + unitsDigit; // Store minutes in decimal

        tensDigit = gsmResponse[23] - 48;
        tensDigit = tensDigit * 10;
        unitsDigit = gsmResponse[24] - 48;
        currentSeconds = tensDigit + unitsDigit; // Store minutes in decimal
    }
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("getDateFromGSM_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/************************Get Current Clock Time From GSM#End************************************/


/************************Calculate Next Due Dates for Valve Action#Start************************************/

/*************************************************************************************************************************

This function is called to get due date
The purpose of this function is to calculate the future date with given days from current date

 **************************************************************************************************************************/
void getDueDate(unsigned char days) {
    unsigned int remDays = CLEAR, offset = CLEAR, leapYearDays = 366, yearDays = 365;
    unsigned char firstMonth = 1, lastMonth = 12, month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("getDueDate_IN\r\n");
    //********Debug log#end**************//
#endif
    dueDD = CLEAR, dueMM = CLEAR, dueYY = CLEAR;
    __delay_ms(100);
    fetchTimefromRTC();
    __delay_ms(100);
    dueDD = currentDD; // get todays day and set as temporary  dueDD
    switch (currentMM - 1) {
        case 11:
            dueDD += 30;
        case 10:
            dueDD += 31;
        case 9:
            dueDD += 30;
        case 8:
            dueDD += 31;
        case 7:
            dueDD += 31;
        case 6:
            dueDD += 30;
        case 5:
            dueDD += 31;
        case 4:
            dueDD += 30;
        case 3:
            dueDD += 31;
        case 2:
            dueDD += 28;
        case 1:
            dueDD += 31;
    }
    // leap year and greater than February
    if ((((2000 + (unsigned int) currentYY) % 100 != 0 && currentYY % 4 == 0) || (2000 + (unsigned int) currentYY) % 400 == 0) && currentMM > 2) {
        dueDD += 1;
    }
    //leap year
    if (((2000 + (unsigned int) currentYY) % 100 != 0 && currentYY % 4 == 0) || (2000 + (unsigned int) currentYY) % 400 == 0) {
        remDays = leapYearDays - dueDD;
    } else {
        remDays = yearDays - dueDD;
    }
    if (days <= remDays) {
        dueYY = currentYY;
        dueDD += days;
    } else {
        days -= remDays;
        dueYY = currentYY + 1;
        //leap year
        if (((2000 + (unsigned int) dueYY) % 100 != 0 && dueYY % 4 == 0) || (2000 + (unsigned int) dueYY) % 400 == 0) {
            offset = leapYearDays;
        } else {
            offset = yearDays;
        }
        while (days >= offset) {
            days -= offset;
            dueYY++;
            //leap year
            if (((2000 + (unsigned int) dueYY) % 100 != 0 && dueYY % 4 == 0) || (2000 + (unsigned int) dueYY) % 400 == 0) {
                offset = leapYearDays;
            } else {
                offset = yearDays;
            }
        }
        dueDD = days;
    }
    //leap year
    if (((2000 + (unsigned int) currentYY) % 100 != 0 && currentYY % 4 == 0) || (2000 + (unsigned int) currentYY) % 400 == 0) {
        month[2] = 29;
    }
    for (dueMM = firstMonth; dueMM <= lastMonth; dueMM++) {
        if (dueDD <= month[dueMM])
            break;
        dueDD = dueDD - month[dueMM];
    }
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("getDueDate_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/************************Calculate Next Due Dates for Valve Action#End************************************/


/************************SleepCount for Next Valve Action#Start************************************/

/*************************************************************************************************************************

This function is called to get sleep count
The purpose of this function is to check if configured valve is due then calculate sleep count for on period else calculate sleep count upto the nearest configured valve setting.

 **************************************************************************************************************************/
void scanValveScheduleAndGetSleepCount(void) {
    unsigned long newCount = CLEAR; // Used to save temporary calculated sleep count
    unsigned int leapYearDays = 366, yearDays = 365;
    unsigned char iLocal = CLEAR;
    _Bool fieldCylceChecked = false;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("scanValveScheduleAndGetSleepCount_IN\r\n");
    //********Debug log#end**************//
#endif
    sleepCount = 65500; // Set Sleep count to default value until it is calculated
    currentDateCalled = false;
    if (startFieldNo > 11) {
        startFieldNo = 0;
    }
nxtCycle:
    for (iterator = startFieldNo; iterator < fieldCount; iterator++) {
        // do if Configured valve is not in action
        if (fieldValve[iterator].isConfigured && fieldValve[iterator].status != ON) {
            //get current date only for one iteration
            if (!currentDateCalled) {
                __delay_ms(100);
                fetchTimefromRTC(); // Get today's date
                __delay_ms(100);
                currentDateCalled = true; // Today's date is known
                sleepCount = 65500; // Set Sleep count to default value until it is calculated
            }
            /*** Due date is over passed without taking action on valves ***/
            // if year over passes || if month  over passes || if day over passes || if hour over passes ||if minute over passes
            if ((currentYY > fieldValve[iterator].nextDueYY) || (currentMM > fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY) || (currentDD > fieldValve[iterator].nextDueDD && currentMM == fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY) || (currentHour > fieldValve[iterator].motorOnTimeHour && currentDD == fieldValve[iterator].nextDueDD && currentMM == fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY) || (currentMinutes >= fieldValve[iterator].motorOnTimeMinute && currentHour == fieldValve[iterator].motorOnTimeHour && currentDD == fieldValve[iterator].nextDueDD && currentMM == fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY)) {
                valveDue = true; // Set Value Due
                break;
            } else if (fieldValve[iterator].cyclesExecuted < fieldValve[iterator].cycles) {
                valveDue = true; // Set Value Due
                break;
            }// Due Date is yet to come find the sleep count to reach the Due date	
            else {
                valveDue = false; // All due valves are operated
                newCount = CLEAR; // clear initial temporary calculated sleep count

                /*** temporary sleep count between today's date and valve's next due date ***/

                for (iLocal = currentYY; iLocal < fieldValve[iterator].nextDueYY; iLocal++) {
                    if ((2000 + (unsigned int) iLocal) % 100 != 0 && iLocal % 4 == 0 && (2000 + (unsigned int) iLocal) % 400 == 0)
                        newCount += leapYearDays;
                    else
                        newCount += yearDays;
                }
                newCount += days(fieldValve[iterator].nextDueMM, fieldValve[iterator].nextDueYY);
                newCount += fieldValve[iterator].nextDueDD;
                newCount -= days(currentMM, currentYY);
                newCount -= currentDD;
                newCount *= 24; // converting into no. of hours
                // Consider current hour in calculated sleep count
                if (fieldValve[iterator].motorOnTimeHour >= currentHour) {
                    newCount += (fieldValve[iterator].motorOnTimeHour - currentHour);
                    /****converting in minutes****/
                    newCount *= 60;
                    if (currentMinutes >= fieldValve[iterator].motorOnTimeMinute) {
                        newCount -= (currentMinutes - fieldValve[iterator].motorOnTimeMinute);
                    } else {
                        newCount += (fieldValve[iterator].motorOnTimeMinute - currentMinutes);
                    }
                }// Subtract current hour from calculated sleep count
                else if (fieldValve[iterator].motorOnTimeHour < currentHour) {
                    newCount -= (currentHour - fieldValve[iterator].motorOnTimeHour);
                    /****converting in minutes****/
                    newCount *= 60;
                    if (currentMinutes >= fieldValve[iterator].motorOnTimeMinute) {
                        newCount -= (currentMinutes - fieldValve[iterator].motorOnTimeMinute);
                    } else {
                        newCount += (fieldValve[iterator].motorOnTimeMinute - currentMinutes);
                    }
                }
                // Valve is due in a minute
                if (newCount == 0 || newCount == 1) {
                    sleepCount = 1; // calculate sleep count for upcoming due valve
                }// Save sleep count for nearest next valve action  
                else if (newCount < sleepCount) {
                    sleepCount = (unsigned int) newCount; // calculate sleep count for upcoming due valve
                }
            }
        }
    }
    if (!valveDue && !fieldCylceChecked) {
        fieldCylceChecked = true;
        startFieldNo = 0; // Reset start field no after scanning all irrigation valves from start field no.
        goto nxtCycle;
    }
    if (valveDue) {
        /* check Fertigation status and set sleep count to fertigation wet period*/
        if (fieldValve[iterator].isFertigationEnabled && fieldValve[iterator].fertigationInstance != 0) {
            sleepCount = fieldValve[iterator].fertigationDelay; // calculate sleep count for fertigation delay 
            fieldValve[iterator].fertigationStage = wetPeriod;
            saveFertigationValveStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
#ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("scanValveScheduleAndGetSleepCount_ValveDueWithFertigation_OUT\r\n");
            //********Debug log#end**************//
#endif
        }/*Only Irrigation valve*/
        else {
            sleepCount = fieldValve[iterator].onPeriod; // calculate sleep count for Valve on period 
#ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("scanValveScheduleAndGetSleepCount_ValveDueW/OFertigation_OUT\r\n");
            //********Debug log#end**************//
#endif   
        }
    }
    else {
        if (sleepCount > 1 && sleepCount < 4369) {
            sleepCount = sleepCount * 15;
            sleepCount = (sleepCount / 17);
        }
        else if (sleepCount >= 4369) {
            sleepCount = 4095;
        }
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("scanValveScheduleAndGetSleepCount_W/OValveDue_OUT\r\n");
        //********Debug log#end**************//
#endif
    }
}
/************************SleepCount for Next Valve Action#End************************************/


/************************Days Between Two Dates#Start************************************/

/*************************************************************************************************************************

This function is called to get no. of days.
The purpose of this function is to calculate no. of days left in the calender year from given month and year
It counts no.of days from 1st January of calender to 1st day of given month
 **************************************************************************************************************************/
unsigned int days(unsigned char mm, unsigned char yy) {
    unsigned char itr = CLEAR, month[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    unsigned int days = CLEAR;
    for (itr = 0; itr < mm - 1; itr++) {
        if (itr == 1) {
            if ((2000 + (unsigned int) yy) % 100 != 0 && yy % 4 == 0 && (2000 + (unsigned int) yy) % 400 == 0)
                days += 29;
            else
                days += 28;
        } else
            days += month[itr];
    }
    return (days);
}
/************************Days Between Two Dates#End************************************/


/************************Extract field no. in msg#Start************************************/

/*************************************************************************************************************************

This function is called to fetch field no mentioned in sms.
The purpose of this function is to navigate through received sms and fetch field no from given position 
It fetches two digit field no. from given position
 **************************************************************************************************************************/
unsigned char fetchFieldNo(unsigned char index) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("fetchFieldNo_IN: ");
    //********Debug log#end**************//
#endif
    if (decodedString[index] == '0' && decodedString[index + 1] == '1') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 49; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 0;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '2') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 50; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 1;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '3') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 51; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 2;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '4') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 52; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 3;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '5') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 53; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 4;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '6') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 54; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 5;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '7') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 55; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 6;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '8') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 56; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 7;
    } else if (decodedString[index] == '0' && decodedString[index + 1] == '9') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = 57; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 8;
    } else if (decodedString[index] == '1' && decodedString[index + 1] == '0') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = 48; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 9;
    } else if (decodedString[index] == '1' && decodedString[index + 1] == '1') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = 49; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 10;
    } else if (decodedString[index] == '1' && decodedString[index + 1] == '2') {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = 50; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 11;
    } else {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        temporaryBytesArray[0] = 57; // To store field no. of valve in action 
        temporaryBytesArray[1] = 57; // To store field no. of valve in action 
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\nfetchFieldNo_OUT\r\n");
        //********Debug log#end**************//
#endif
        return 255;
    }

}
/************************Extract field no. in msg#End************************************/


/************************Actions on Received SMS#Start************************************/

/*************************************************************************************************************************

This function is called to extract received message after new message notification is received.
The purpose of this function is to read sms received at 1st SIM memory location.
The Action is decided upon Type of message received.

 **************************************************************************************************************************/
void extractReceivedSms(void) {
    unsigned char count = CLEAR, onHour = CLEAR, onMinute = CLEAR;
    unsigned int digit = CLEAR;
    timer3Count = 30; // 30 sec window
#ifdef LCD_DISPLAY_ON_H
    lcdClear();
    lcdWriteStringAtCenter("New Message Received", 2);
    __delay_ms(1000);
#endif
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("extractReceivedSms_IN\r\n");
    //********Debug log#end**************//
#endif
    // check for valid sim storage location
    if (temporaryBytesArray[0] > '0' && temporaryBytesArray[0] <= '9') {
        controllerCommandExecuted = false;
        msgIndex = CLEAR;
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
        //setBCDdigit(0x03, 1); // (3) BCD indication for Read Sms Action
        while (!controllerCommandExecuted) {
            transmitStringToGSM("AT+CMGR=");
            transmitNumberToGSM(temporaryBytesArray, 1);
            transmitStringToGSM("\r\n"); // reading msg from sim location
            __delay_ms(500);
            if (!controllerCommandExecuted) {
                __delay_ms(2500);
                __delay_ms(2500);
            }
        }
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
        PIR5bits.TMR3IF = SET; //Stop timer thread  
        // ADD indication if infinite
        if (strncmp(gsmResponse + 21, countryCode, 3) == 0) {
            strncpy(temporaryBytesArray, gsmResponse + 24, 10); // Save received sender no. as temp user
            deleteStringToDecode();
            /*Decode received  Base64 format message*/
#ifdef Encryption_ON_H
            strcpyCustom((char *) stringToDecode, (const char *) gsmResponse + 63);
#ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug((const char *) gsmResponse + 63);
            transmitStringToDebug("\r\n");
            transmitStringToDebug((char *) stringToDecode);
            transmitStringToDebug("\r\n");
            //********Debug log#end**************//
#endif
            deleteGsmResponse();
            if (isBase64String((unsigned char *) stringToDecode)) {
                deleteDecodedString();
                base64Decoder();
            } else {
                //deleteGsmResponse();
                //setBCDdigit(0x05, 0); // (5.) BCD indication for Incorrect SMS format
                __delay_ms(2000);
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("extractReceivedSms_NotBase64String_OUT\r\n");
                //********Debug log#end**************//
#endif
                return;
            }
#endif

#ifndef Encryption_ON_H
            strcpyCustom((char *) decodedString, (const char *) gsmResponse + 63); // w/o encoder
#endif
            //......Types of Message Received from Registered User.........//
            if (strncmp(userMobileNo, temporaryBytesArray, 10) == 0) {
                strncpy(temporaryBytesArray, null, 10);
                //#2>.............Connection message...............//
                // if received connection command is from userMobileNo
                if (strncmp(decodedString, hook, 4) == 0) {
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("User Logged IN", 2);
                    lcdWriteStringAtCenter("System", 3);
#endif
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsConnect, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Connect_OUT\r\n");
                    //********Debug log#end**************// 
#endif
                    return;
                }//#3>..............Change Password.................// 
                    // set Admin format (AU 123456 666666)
                else if (strncmp(decodedString, admin, 2) == 0) {
                    if (strncmp(decodedString + 3, pwd, 6) == 0) {
#ifdef LCD_DISPLAY_ON_H
                        lcdClear();
                        lcdWriteStringAtCenter("Login Code", 1);
                        lcdWriteStringAtCenter("Changed", 2);
#endif
                        strncpy(pwd, decodedString + 10, 6);
                        savePasswordIntoEeprom(); //store password into eeprom
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsPwd1, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("extractReceivedSms_Password updated_OUT\r\n");
                        //********Debug log#end**************// 
#endif
                        return;
                    } else {
#ifdef LCD_DISPLAY_ON_H
                        lcdClear();
                        lcdWriteStringAtCenter("Wrong", 1);
                        lcdWriteStringAtCenter("Login Code", 2);
#endif
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsPwd3, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("extractReceivedSms_Password not changed_OUT\r\n");
                        //********Debug log#end**************//
#endif
                        return;
                    }
                }//#4>..............Save New Configured values of Field Valve X .................//
                    //Msg Format---------***SET<FieldNo.><Space><OnPeriod><Space><OffPeriod><Space><MotorOnTimeHr><Space><MotorOnTimeMins><Space><SensorDryValue><Space><SensorWetValue><Space>***------------//
                    //SMS Location---------63--------<72/73>------<74>-----------<77/78>---------<79/80/81>-------------//

                else if (strncmp(decodedString, set, 3) == 0) {
                    iterator = fetchFieldNo(3);
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Irrigation Data", 1);
                    lcdWriteStringAtCenter("Configured for Field", 2);
                    sprintf(temporaryBytesArray,"No: %d",iterator+1);
                    lcdWriteStringAtCenter(temporaryBytesArray, 3);
#endif                            
                    digit = CLEAR;
                    count = CLEAR; //count to extract onperiod, offperiod,motorOnHour,motorOnMinute,DryValue, wetValue; i.e total 9 attributes
                    
                    for (count = 1, msgIndex = 6; count <= 9; msgIndex++) {
                        //is number
                        if (isNumber(decodedString[msgIndex])) {
                            if (decodedString[msgIndex + 1] == space) {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                digit = digit + decodedString[msgIndex];
                            } else {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                decodedString[msgIndex] = decodedString[msgIndex] * 10;
                                digit = digit * 10;
                                digit = digit + decodedString[msgIndex];
                            }
                        } else {
                            switch (count) {
                                case 1: // code to extract on period;
                                    fieldValve[iterator].onPeriod = digit;
                                    digit = CLEAR;
                                    break;
                                case 2: // code to extract off period;
                                    fieldValve[iterator].offPeriod = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 3: // code to extract motorOnTimeHour;
                                    fieldValve[iterator].motorOnTimeHour = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 4: // code to extract motorOnTimeMinute;
                                    fieldValve[iterator].motorOnTimeMinute = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 5: // code to extract dryValue
                                    fieldValve[iterator].dryValue = digit;
                                    digit = CLEAR;
                                    break;
                                case 6: // code to extract wetValue
                                    fieldValve[iterator].wetValue = digit;
                                    digit = CLEAR;
                                    break;
                                case 7: // code to extract priority
                                    fieldValve[iterator].priority = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 8: // code to extract cycles
                                    fieldValve[iterator].cycles = (unsigned char) digit;
                                    fieldValve[iterator].cyclesExecuted = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 9: // code to extract day count;
                                    getDueDate((unsigned char) digit); // Get due dates w.r.t triggered from date
                                    digit = CLEAR;
                                    fieldValve[iterator].nextDueDD = (unsigned char) dueDD;
                                    fieldValve[iterator].nextDueMM = dueMM;
                                    fieldValve[iterator].nextDueYY = dueYY;
                                    fieldValve[iterator].status = OFF;
                                    fieldValve[iterator].isConfigured = true;
                                    fieldValve[iterator].fertigationDelay = 0;
                                    fieldValve[iterator].fertigationONperiod = 0;
                                    fieldValve[iterator].fertigationInstance = 0;
                                    fieldValve[iterator].isFertigationEnabled = false;
                                    fieldValve[iterator].fertigationStage = OFF;
                                    fieldValve[iterator].fertigationValveInterrupted = false;
                                    break;
                            }
                            count++;
                        }
                    }
                    __delay_ms(100);
                    saveIrrigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    saveIrrigationValveDueTimeIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    saveIrrigationValveConfigurationStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    /***************************/
                    // for field no. 01 to 09
                    if (iterator < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (iterator > 8 && iterator < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                    }
                    /***************************/
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsIrr1, userMobileNo, fieldNoRequired); // Acknowledge user about successful Irrigation configuration
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Configure_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#5>..............Hold Field x Irrigation.................//
                    // Hold<x>
                else if (strncmp(decodedString, hold, 4) == 0) {
                    iterator = fetchFieldNo(4);
                    if (fieldValve[iterator].status == ON) {
                        onHold = true;
                    }
                    fieldValve[iterator].isConfigured = false; // configuration hold
                    if (fieldValve[iterator].isFertigationEnabled == true) {
                        fieldValve[iterator].isFertigationEnabled = false; // configuration hold
                        __delay_ms(100);
                        saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                    }
                    saveIrrigationValveConfigurationStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                    __delay_ms(100);
                    /***************************/
                    // for field no. 01 to 09
                    if (iterator < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (iterator > 8 && iterator < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                    }
                    /***************************/
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsIrr2, userMobileNo, fieldNoRequired); // Acknowledge user about successful Irrigation configuration disable action
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Hold_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#6>.............. Enable Fertigation Valve for field.................//
                    //Msg Format---------***Enable<FieldNo.><Space><Delay><Space><ONPeriod><Space><Instance><Space>***------------//
                    //SMS Location---------63--------<69>------<71>-----------<73/74>---------<78/79/80>-------------//

                else if (strncmp(decodedString, enable, 6) == 0) {
                    digit = CLEAR;
                    count = CLEAR; //count to extract delayStart, onPeriod, no. of times
                    iterator = fetchFieldNo(6);
                    if (fieldValve[iterator].isConfigured == DISABLED) {
                        /***************************/
                        // for field no. 01 to 09
                        if (iterator < 9) {
                            temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                            temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                        }// for field no. 10 to 12
                        else if (iterator > 8 && iterator < 12) {
                            temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                            temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                        }
                        /***************************/
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsFert1, userMobileNo, fieldNoRequired); // Acknowledge user about Fertigation not configured due to disabled irrigation
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    } else {
                        for (msgIndex = 9; count < 15; msgIndex++) {
                            if (isNumber(decodedString[msgIndex])) {
                                if (decodedString[msgIndex + 1] == space) {
                                    decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                    digit = digit + decodedString[msgIndex];
                                } else {
                                    decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                    decodedString[msgIndex] = decodedString[msgIndex] * 10;
                                    digit = digit * 10;
                                    digit = digit + decodedString[msgIndex];
                                }
                            } else {
                                count++;
                                switch (count) {
                                    case 1: // code to extract fertigationDelay;
                                        fieldValve[iterator].fertigationDelay = digit;
                                        digit = CLEAR;
                                        break;
                                    case 2: // code to extract fertigationONperiod;
                                        fieldValve[iterator].fertigationONperiod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 3: // code to extract fertigationInstance;
                                        fieldValve[iterator].fertigationInstance = (unsigned char) digit;
                                        digit = CLEAR;
                                        break;
                                    case 4: // code to extract injector1OnPeriod;
                                        fieldValve[iterator].injector1OnPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 5: // code to extract injector1OffPeriod;
                                        fieldValve[iterator].injector1OffPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 6: // code to extract injector1Cycle;
                                        fieldValve[iterator].injector1Cycle = (unsigned char) digit;
                                        digit = CLEAR;
                                        break;
                                    case 7: // code to extract injector2OnPeriod;
                                        fieldValve[iterator].injector2OnPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 8: // code to extract injector2OffPeriod;
                                        fieldValve[iterator].injector2OffPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 9: // code to extract injector2Cycle;
                                        fieldValve[iterator].injector2Cycle = (unsigned char) digit;
                                        digit = CLEAR;
                                        break;
                                    case 10: // code to extract injector3OnPeriod;
                                        fieldValve[iterator].injector3OnPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 11: // code to extract injector3OffPeriod;
                                        fieldValve[iterator].injector3OffPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 12: // code to extract injector3Cycle;
                                        fieldValve[iterator].injector3Cycle = (unsigned char) digit;
                                        digit = CLEAR;
                                        break;
                                    case 13: // code to extract injector4OnPeriod;
                                        fieldValve[iterator].injector4OnPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 14: // code to extract injector4OffPeriod;
                                        fieldValve[iterator].injector4OffPeriod = digit;
                                        digit = CLEAR;
                                        break;
                                    case 15: // code to extract injector1Cycle;
                                        fieldValve[iterator].injector4Cycle = (unsigned char) digit;
                                        fieldValve[iterator].fertigationStage = OFF;
                                        fieldValve[iterator].fertigationValveInterrupted = false;
                                        digit = CLEAR;
                                        if ((fieldValve[iterator].fertigationDelay + fieldValve[iterator].fertigationONperiod) >= fieldValve[iterator].onPeriod) {
                                            fieldValve[iterator].isFertigationEnabled = false;
                                            /***************************/
                                            // for field no. 01 to 09
                                            if (iterator < 9) {
                                                temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                                                temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                                            }// for field no. 10 to 12
                                            else if (iterator > 8 && iterator < 12) {
                                                temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                                                temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                                            }
                                            /***************************/
                                            msgIndex = CLEAR;
                                            /***************************/
                                            sendSms(SmsFert2, userMobileNo, fieldNoRequired); // Acknowledge user about Fertigation not configured due to incorrect values
#ifdef SMS_DELIVERY_REPORT_ON_H
                                            sleepCount = 2; // Load sleep count for SMS transmission action
                                            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                                            //setBCDdigit(0x05, 0);
                                            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                                            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                                            /***************************/
                                        } else {
                                            fieldValve[iterator].isFertigationEnabled = true;
                                        }
                                        break;
                                }
                            }
                        }
                        if (fieldValve[iterator].isFertigationEnabled) {
                            __delay_ms(100);
                            saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                            __delay_ms(100);
                            /***************************/
                            // for field no. 01 to 09
                            if (iterator < 9) {
                                temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                                temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                            }// for field no. 10 to 12
                            else if (iterator > 8 && iterator < 12) {
                                temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                                temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                            }
                            /***************************/
                            msgIndex = CLEAR;
                            /***************************/
                            sendSms(SmsFert3, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation enabled action
#ifdef SMS_DELIVERY_REPORT_ON_H
                            sleepCount = 2; // Load sleep count for SMS transmission action
                            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                            //setBCDdigit(0x05, 0);
                            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                            /***************************/
                        }
                    }
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_EnableFertigation_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#7>..............Disable Field x fertigation.................//
                    // DISABLE<x>
                else if (strncmp(decodedString, disable, 7) == 0) {
                    iterator = fetchFieldNo(7);
                    if (fieldValve[iterator].status == ON && fieldValve[iterator].fertigationStage != flushPeriod) {
                        onHold = true;
                    }
                    if (fieldValve[iterator].isFertigationEnabled == true) {
                        fieldValve[iterator].isFertigationEnabled = false; // configuration hold
                        __delay_ms(100);
                        saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        /***************************/
                        // for field no. 01 to 09
                        if (iterator < 9) {
                            temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                            temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                        }// for field no. 10 to 12
                        else if (iterator > 8 && iterator < 12) {
                            temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                            temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                        }
                        /***************************/
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsFert4, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation disabled action
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("extractReceivedSms_Hold_OUT\r\n");
                        //********Debug log#end**************//
#endif
                        return;
                    }
                }//#8>..............Activate Filtration.................//
                    //Msg Format---------***ACTIVE<Delay1><Space><Delay2><Space><Delay3><Space><OnTime><Space><SeparationTime><Space>***------------//

                else if (strncmp(decodedString, active, 6) == 0) {
                    digit = CLEAR;
                    count = CLEAR; //count to extract Timestamp (total 6 attributes)                
                    for (msgIndex = 6; count < 5; msgIndex++) {
                        //is number
                        if (isNumber(decodedString[msgIndex])) {
                            if (decodedString[msgIndex + 1] == space) {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                digit = digit + decodedString[msgIndex];
                            } else {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                decodedString[msgIndex] = decodedString[msgIndex] * 10;
                                digit = digit * 10;
                                digit = digit + decodedString[msgIndex];
                            }
                        } else {
                            count++;
                            switch (count) {
                                case 1: // code to extract filtrationDelay1;
                                    filtrationDelay1 = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 2: // code to extract filtrationDelay2;
                                    filtrationDelay2 = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 3: // code to extract filtrationDelay3;
                                    filtrationDelay3 = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 4: // code to extract filtrationOnTime;
                                    filtrationOnTime = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 5: // code to filtrationSeperationTime
                                    filtrationSeperationTime = (unsigned char) digit;
                                    digit = CLEAR;
                                    filtrationEnabled = true;
                                    break;
                            }
                        }
                    }
                    __delay_ms(100);
                    saveFiltrationSequenceData();
                    __delay_ms(100);

                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsFilt1, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Configure_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#9>..............Disable filtration.................//
                    // DACTIVE
                else if (strncmp(decodedString, dactive, 7) == 0) {
                    filtrationEnabled = false; // filtration disabled
                    __delay_ms(100);
                    saveFiltrationSequenceData();
                    __delay_ms(100);
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsFilt2, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Hold_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#10>..............get filtration data.................//
                    // FDATA
                else if (strncmp(decodedString, fdata, 5) == 0) {
                    if (filtrationEnabled) {
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsFilt4, userMobileNo, filtrationData);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    } else {
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsFilt3, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    }
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_sendDiagnosticData_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#11>..............Set RTC Time.................//
                    //Msg Format---------***Feed<Space><DD><Space><MM><Space><YY><Space><Hr><Space><Min><Space><Sec>***------------//
                    //SMS Location----------63---------<68>-------<71>-------<74>-------<77>-------<80>------<83>//


                else if (strncmp(decodedString, feed, 4) == 0) {
                    digit = CLEAR;
                    count = CLEAR; //count to extract Timestamp (total 6 attributes)                
                    for (msgIndex = 5; count < 6; msgIndex += 3) {
                        //is number
                        if (isNumber(decodedString[msgIndex])) {
                            decodedString[msgIndex] = decodedString[msgIndex] - 48;
                            digit = decodedString[msgIndex];
                            digit = digit * 10;
                            decodedString[msgIndex + 1] = decodedString[msgIndex + 1] - 48;
                            digit = digit + decodedString[msgIndex + 1];
                            count++;
                            switch (count) {
                                case 1: // code to extract DD;
                                    currentDD = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 2: // code to extract mm;
                                    currentMM = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 3: // code to extract YY;
                                    currentYY = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 4: // code to extract Hour;
                                    currentHour = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 5: // code to extract Minutes;
                                    currentMinutes = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 6: // code to extract Seconds;
                                    currentSeconds = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                            }
                        } else {
                            msgIndex = CLEAR;
                            /***************************/
                            sendSms(SmsT1, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                            sleepCount = 2; // Load sleep count for SMS transmission action
                            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                            //setBCDdigit(0x05, 0);
                            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                            /***************************/
#ifdef DEBUG_MODE_ON_H
                            //********Debug log#start************//
                            transmitStringToDebug("extractReceivedSms_Configure_OUT\r\n");
                            //********Debug log#end**************//
#endif
                            return;
                        }
                    }
                    if (count == 6) {
                        feedTimeInRTC();
                    }
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsRTC2, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Configure_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#12>..............Get RTC Time.................//
                    // Get current Time
                else if (strncmp(decodedString, time, 4) == 0) {
                    fetchTimefromRTC(); // Get today's date
                    temporaryBytesArray[0] = (currentDD / 10) + 48;
                    temporaryBytesArray[1] = (currentDD % 10) + 48;
                    temporaryBytesArray[2] = '/';
                    temporaryBytesArray[3] = (currentMM / 10) + 48;
                    temporaryBytesArray[4] = (currentMM % 10) + 48;
                    temporaryBytesArray[5] = '/';
                    temporaryBytesArray[6] = (currentYY / 10) + 48;
                    temporaryBytesArray[7] = (currentYY % 10) + 48;
                    temporaryBytesArray[8] = ' ';
                    temporaryBytesArray[9] = (currentHour / 10) + 48;
                    temporaryBytesArray[10] = (currentHour % 10) + 48;
                    temporaryBytesArray[11] = ':';
                    temporaryBytesArray[12] = (currentMinutes / 10) + 48;
                    temporaryBytesArray[13] = (currentMinutes % 10) + 48;
                    temporaryBytesArray[14] = ':';
                    temporaryBytesArray[15] = (currentSeconds / 10) + 48;
                    temporaryBytesArray[16] = (currentSeconds % 10) + 48;
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsT2, userMobileNo, timeRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Current Time_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#13>.............. Extract configured Data for Self Diagnostic.................//
                    // extract<x>
                else if (strncmp(decodedString, extract, 7) == 0) {
                    iterator = fetchFieldNo(7);
                    /***************************/
                    // for field no. 01 to 09
                    if (iterator < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (iterator > 8 && iterator < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                    }
                    /***************************/
                    msgIndex = CLEAR;
                    if (fieldValve[iterator].isConfigured) {
                        /***************************/
                        sendSms(SmsIrr7, userMobileNo, IrrigationData); // Give diagnostic data
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    } else {
                        /***************************/
                        sendSms(SmsIrr3, userMobileNo, fieldNoRequired); // Acknowledge user about  Irrigation not configured
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    }
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_sendDiagnosticData_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#14>......Set up Motor load condition Manually .......//
                    // CT<Space><No Load Cutoff>Space<Full Load Cutoff><Space>
                else if (strncmp(decodedString, ct, 2) == 0) {
                    if (Irri_Motor == ON) {
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsMotor4, userMobileNo, noInfo); // Acknowledge user about Irrigation is active, Motor load cut-off procedure not started
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("Motor load values set successfully_OUT\r\n");
                        //********Debug log#end**************//
#endif
                        return;
                    }
                    digit = CLEAR;
                    count = CLEAR; //count to extract motor load condition (total 2 attributes)
                    for (count = 1, msgIndex = 3; count <= 2; msgIndex++) {
                        //is number
                        if (isNumber(decodedString[msgIndex])) {
                            if (decodedString[msgIndex + 1] == space) {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                digit = digit + decodedString[msgIndex];
                            } else {
                                decodedString[msgIndex] = decodedString[msgIndex] - 48;
                                decodedString[msgIndex] = decodedString[msgIndex] * 10;
                                digit = digit * 10;
                                digit = digit + decodedString[msgIndex];
                            }
                        } else {
                            switch (count) {
                                case 1: // code to extract on period;
                                    noLoadCutOff = digit;
                                    digit = CLEAR;
                                    break;
                                case 2: // code to extract off period;
                                    fullLoadCutOff = digit;
                                    digit = CLEAR;
                                    break;
                            }
                            count++;
                        }
                    }
                    __delay_ms(100);
                    saveMotorLoadValuesIntoEeprom();
                    __delay_ms(100);
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsMotor2, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("Motor load values set successfully_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#15>......GetCTValues for fetching Motor load values.......//
                else if (strncmp(decodedString, getct, 5) == 0) {
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsMotor3, userMobileNo, motorLoadRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission	
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSMS_Get CT Values_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#16>......Get FREQUENCY Values for field moisture sensors.......//
                else if (strncmp(decodedString, getfreq, 7) == 0) {
                    iterator = fetchFieldNo(7);
                    __delay_ms(100);
                    isFieldMoistureSensorWetLora(iterator);
                    /***************************/
                    // for field no. 01 to 09
                    if (iterator < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (iterator > 8 && iterator < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                    }
                    /***************************/
                    if (moistureSensorFailed) {
                        moistureSensorFailed = false;
                        /***************************/
                        sendSms(SmsMS3, userMobileNo, fieldNoRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    } else {
                        /***************************/
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsMS2, userMobileNo, frequencyRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/

                    }

#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_sendMoistureSensorData_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#17>......Set up Motor load condition Automatically.......//
                    // SETCT<FieldNno.>
                else if (strncmp(decodedString, setct, 3) == 0) {
                    iterator = fetchFieldNo(3);
                    __delay_ms(100);
                    if (Irri_Motor == ON) {
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsMotor4, userMobileNo, noInfo); // Acknowledge user about Irrigation is active, Motor load cut-off procedure not started
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("Motor load values set successfully_OUT\r\n");
                        //********Debug log#end**************//
#endif
                        return;
                    }
                    /***************************/
                    // for field no. 01 to 09
                    if (iterator < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (iterator > 8 && iterator < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                    }
                    __delay_ms(2500);
                    __delay_ms(2500);
                    //calibrateMotorCurrent(NoLoad, iterator);
                    //__delay_ms(1000);
                    calibrateMotorCurrent(FullLoad, iterator);
                    __delay_ms(1000);
                    saveMotorLoadValuesIntoEeprom();
                    __delay_ms(1000);
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsMotor3, userMobileNo, motorLoadRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("Motor load values set successfully_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }//#18>......InjectTestData.......//
                    // INJECT<Space><HR><Space><MIN><Space><trigger>
                else if (strncmp(decodedString, inject, 6) == 0) {
                    digit = CLEAR;
                    count = CLEAR; //count to extract Timestamp (total 6 attributes)                
                    for (msgIndex = 7; count < 3; msgIndex += 3) {
                        //is number
                        if (isNumber(decodedString[msgIndex])) {
                            decodedString[msgIndex] = decodedString[msgIndex] - 48;
                            digit = decodedString[msgIndex];
                            digit = digit * 10;
                            decodedString[msgIndex + 1] = decodedString[msgIndex + 1] - 48;
                            digit = digit + decodedString[msgIndex + 1];
                            count++;
                            switch (count) {
                                case 1: // code to extract motorOnTimeHour;
                                    onHour = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 2: // code to extract motorOnTimeMinute;
                                    onMinute = (unsigned char) digit;
                                    digit = CLEAR;
                                    break;
                                case 3: // code to extract day;
                                    getDueDate((unsigned char) digit); // Get due dates w.r.t today's date
                                    digit = CLEAR;
                                    break;
                            }
                        }
                    }
                    for (iterator = 0; iterator < fieldCount; iterator++) {
                        fieldValve[iterator].onPeriod = 15;
                        fieldValve[iterator].offPeriod = 1;
                        fieldValve[iterator].motorOnTimeHour = onHour;
                        fieldValve[iterator].motorOnTimeMinute = onMinute;
                        fieldValve[iterator].nextDueDD = (unsigned char) dueDD;
                        fieldValve[iterator].nextDueMM = dueMM;
                        fieldValve[iterator].nextDueYY = dueYY;
                        fieldValve[iterator].dryValue = 100;
                        fieldValve[iterator].wetValue = 30000;
                        fieldValve[iterator].priority = iterator + 1;
                        fieldValve[iterator].status = OFF;
                        fieldValve[iterator].cycles = 2;
                        fieldValve[iterator].cyclesExecuted = 2;
                        fieldValve[iterator].isConfigured = true;
                        fieldValve[iterator].fertigationDelay = 5;
                        fieldValve[iterator].fertigationONperiod = 15;
                        fieldValve[iterator].fertigationInstance = iterator + 1;
                        fieldValve[iterator].fertigationStage = OFF;
                        fieldValve[iterator].fertigationValveInterrupted = false;
                        fieldValve[iterator].isFertigationEnabled = true;

                        saveIrrigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        saveIrrigationValveDueTimeIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        saveIrrigationValveConfigurationStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                        saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
                        __delay_ms(100);
                    }
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsTest, userMobileNo, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("InjectTestData_Admin set successfully_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                }
                //#19>......MapFieldData.......//
                //               4,5     6,7     8,9     10,11     12,13   14,15   16,17    18,19    20,21
                // MAP<Space><FieldNo.><SlaveNo><IONo.><FieldNo.><SlaveNo><IONo.><FieldNo.><SlaveNo><IONo.><Space>
                else if (strncmp(decodedString, map, 3) == 0) {
                    unsigned char index = 4;
                    while (decodedString[index] != space) {
                        iterator = fetchFieldNo(index);
                        sprintf(temporaryBytesArray,"No: %d",iterator+1);
                        lcdWriteStringAtCenter(temporaryBytesArray, 4);
                        __delay_ms(1000);
                        fieldMap[iterator*2] = fetchFieldNo(index+2) + 1;// Slave No.
                        sprintf(temporaryBytesArray,"No: %d",fieldMap[iterator*2]);
                        lcdWriteStringAtCenter(temporaryBytesArray, 4);
                        __delay_ms(1000);
                        fieldMap[(iterator*2)+1] = fetchFieldNo(index+4) + 1;// IO Pin No.
                        sprintf(temporaryBytesArray,"No: %d",fieldMap[(iterator*2)+1]);
                        lcdWriteStringAtCenter(temporaryBytesArray, 4);
                        __delay_ms(1000);
                        index = index +6;
                    }
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Field Mapping", 2);
                    lcdWriteStringAtCenter("Completed", 3);
#endif                    
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsIrr10, userMobileNo, newAdmin); //To notify old Admin about new Admin.
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                }
            }//#1>......Register New Admin.......//
                // set admin format (AU 123456)
            else if (strncmp(decodedString, admin, 2) == 0) {
                if (strncmp(decodedString + 3, pwd, 6) == 0) {
                    // if not first time installation
                    if (systemAuthenticated) {
#ifdef LCD_DISPLAY_ON_H
                        lcdClear();
                        lcdWriteStringAtCenter("User Registration", 2);
                        lcdWriteStringAtCenter("Completed", 3);
#endif                    
                        msgIndex = CLEAR;
                        /***************************/
                        sendSms(SmsAU2, userMobileNo, newAdmin); //To notify old Admin about new Admin.
#ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05, 0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                        /***************************/
                    }
                    strncpy(pwd, decodedString + 10, 6);
                    strncpy(userMobileNo, temporaryBytesArray, 10);
                    systemAuthenticated = true;
                    strncpy(temporaryBytesArray, null, 10);
                    __delay_ms(50);
                    savePasswordIntoEeprom();
                    __delay_ms(50);
                    saveMobileNoIntoEeprom();
                    __delay_ms(50);
                    saveAuthenticationStatus();
                    __delay_ms(50);
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsAU1, userMobileNo, noInfo); // Acknowledge user about successful Admin Registration
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Admin set successfully_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    return;
                } else {
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Failed User", 1);
                    lcdWriteStringAtCenter("Registration", 2);
#endif
                    msgIndex = CLEAR;
                    /***************************/
                    sendSms(SmsPwd3, temporaryBytesArray, noInfo);
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
#ifdef DEBUG_MODE_ON_H
                    //********Debug log#start************//
                    transmitStringToDebug("extractReceivedSms_Wrong Password_OUT\r\n");
                    //********Debug log#end**************//
#endif
                    strncpy(temporaryBytesArray, null, 10);
                    return;
                }
            }//#0>......Connect from non admin.......//
            else if (strncmp(decodedString, hook, 4) == 0) {
#ifdef LCD_DISPLAY_ON_H
                lcdClear();
                lcdWriteStringAtCenter("Authentication", 1);
                lcdWriteStringAtCenter("Failed", 2);
#endif
                msgIndex = CLEAR;
                /***************************/
                sendSms(SmsAU3, temporaryBytesArray, noInfo); // Acknowledge user about Authentication failed
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission	
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("extractReceivedSms_Not Authenticated_OUT\r\n");
                //********Debug log#end**************//
#endif
                strncpy(temporaryBytesArray, null, 10);
                return;
            }//#-1>......Secret code for fetching Factory Password.......//
            else if (strncmp(decodedString, secret, 11) == 0) {
                msgIndex = CLEAR;
                /***************************/
                sendSms(SmsFact1, temporaryBytesArray, secretCodeRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission	
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("extractReceivedSms_Factory Password_OUT\r\n");
                //********Debug log#end**************//
#endif
                strncpy(temporaryBytesArray, null, 10);
                return;
            } else {
                //setBCDdigit(0x06, 0); // (6.) BCD indication for unknown sms from registered user
                __delay_ms(2000);
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("extractReceivedSms_UnknownSMS_fromRegisteredUser_OUT\r\n");
                //********Debug log#end**************//
#endif
                return;
            }
        } else {
            deleteGsmResponse();
            //setBCDdigit(0x07, 0); // (7.) BCD indication for sms from non gsm no.
            __delay_ms(1000);
            /***************************/
#ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("extractReceivedSms_NotFromGSM_OUT\r\n");
            //********Debug log#end**************//
#endif
            return;
        }
    } else {
        deleteGsmResponse();
        //setBCDdigit(0x08, 0); // (8.) BCD indication for Incorrect SMS storage no.
        __delay_ms(2000);
        /***************************/
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("extractReceivedSms_IncorrectSMS_Storage_OUT\r\n");
        //********Debug log#end**************//
#endif
        return;
    }
}
/************************Actions on Received SMS#End************************************/


/*********** Moisture sensor measurement#Start********/

/*************************************************************************************************************************

This function is called to measure soil moisture of given field and indicate if wet field found .
The Moisture level is measured in terms of frequency of square wave generated by IC555 based on Senor resistance.
The Sensor resistance is high and low for Dry and Wet condition respectively.
This leads the output of IC555 with high and low pulse width.
For Dry condition pulse width is high and for wet condition pulse width is low.
i.e. for Dry condition pulse occurrence is low and for wet condition pulse occurrence is high
Here Timer1 is used to count frequency of pulses by measuring timer count for 1 pulse width and averaging it for 10 pulses.

 **************************************************************************************************************************/
_Bool isFieldMoistureSensorWet() {
    unsigned long moistureLevelAvg = CLEAR;
    unsigned long timer1Value = CLEAR; // To store 16 bit SFR Timer1 Register value
    unsigned long constant = 160000; // Constant to calculate frequency in Hz ~16MHz 16000000/100 to convert freq from 5 digit to 3 digit
    unsigned char itr = CLEAR, avg = 20;

    moistureLevel = CLEAR; // To store moisture level in Hz

#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    /***************************/
    // for field no. 01 to 09
    if (FieldNo < 9) {
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
    }// for field no. 10 to 12
    else if (FieldNo > 8 && FieldNo < 12) {
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
    } else {
        temporaryBytesArray[0] = 57; // To store field no. of valve in action 
        temporaryBytesArray[1] = 57; // To store field no. of valve in action 

    }
    /***************************/
    transmitStringToDebug("isFieldMoistureSensorWet_IN : ");
    transmitNumberToDebug(temporaryBytesArray, 2);
    transmitStringToDebug("\r\n");
    //********Debug log#end**************//
#endif

    //setBCDdigit(0x09, 0); // (9.) BCD indication for Moisture Sensor Failure Error
    moistureLevel = LOW;
    checkMoistureSensor = true;
    moistureSensorFailed = false;
    timer3Count = 5; // 5 second window
    // Averaging measured pulse width
    for (itr = 1; itr <= avg && !moistureSensorFailed; itr++) {
        T1CONbits.TMR1ON = OFF;
        TMR1H = CLEAR;
        TMR1L = CLEAR;
        Timer1Overflow = CLEAR;
        // check field moisture of valve in action

#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("MoistureSensor1 == HIGH\r\n");
        //********Debug log#end**************//
#endif
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if Sensor fails to respond within 15 sec    
        controllerCommandExecuted = false;
        while (Mois_SNSR == HIGH && controllerCommandExecuted == false);
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("MoistureSensor1 == LOW\r\n");
        //********Debug log#end**************//
#endif
        while (Mois_SNSR == LOW && controllerCommandExecuted == false); // Wait for rising edge
        T1CONbits.TMR1ON = ON; // Start Timer after rising edge detected
        while (Mois_SNSR == HIGH && controllerCommandExecuted == false); // Wait for falling edge
        if (!controllerCommandExecuted) {
            controllerCommandExecuted = true;
            PIR5bits.TMR3IF = SET; //Stop timer thread 
        }
        T1CONbits.TMR1ON = OFF; // Stop timer after falling edge detected
        timer1Value = TMR1L; // Store lower byte of 16 bit timer
        timer1Value |= ((unsigned int) TMR1H) << 8; // Get higher and lower byte of  timer1 register
        moistureLevelAvg = (Timer1Overflow * 65536) + timer1Value;
        moistureLevelAvg *= 2; // Entire cycle width
        moistureLevelAvg = (constant / moistureLevelAvg); // Frequency = 16Mhz/ Pulse Width
        if (itr == 1) {
            moistureLevel = (unsigned int) moistureLevelAvg;
        }
        moistureLevel = moistureLevel / 2;
        moistureLevelAvg = moistureLevelAvg / 2;
        moistureLevel = moistureLevel + (unsigned int) moistureLevelAvg;
        if (moistureSensorFailed) {
            moistureLevel = 0;
        }
    }
    checkMoistureSensor = false;
    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
    if (moistureLevel >= 150) {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isFertigationSensorWet_Yes_Out\r\n");
        //********Debug log#end**************//
#endif
        return true;
    }
    else {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isFertigationSensorWet_No_Out\r\n");
        //********Debug log#end**************//
#endif
        return false;
    }
}



/*********** Moisture sensor measurement at Lora Slave #Start********/

/*************************************************************************************************************************

This function is called to send command to lora slave to measure soil moisture of given field and indicate if wet field found .
The Moisture level is measured in terms of frequency of square wave generated by IC555 based on Senor resistance.
The Sensor resistance is high and low for Dry and Wet condition respectively.
This leads the output of IC555 with high and low pulse width.
For Dry condition pulse width is high and for wet condition pulse width is low.
i.e. for Dry condition pulse occurrence is low and for wet condition pulse occurrence is high
Here Timer1 is used to count frequency of pulses by measuring timer count for 1 pulse width and averaging it for 10 pulses.

 **************************************************************************************************************************/
_Bool isFieldMoistureSensorWetLora(unsigned char FieldNo) {
    unsigned char action;
    loraAttempt = 0;
    action = 0x02;
    //setBCDdigit(0x09, 0); // (9.) BCD indication for Moisture Sensor Failure Error
    moistureSensorFailed = false;
    // Averaging measured pulse width

#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    /***************************/
    // for field no. 01 to 09
    if (FieldNo < 9) {
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
    }// for field no. 10 to 12
    else if (FieldNo > 8 && FieldNo < 12) {
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
    } else {
        temporaryBytesArray[0] = 57; // To store field no. of valve in action 
        temporaryBytesArray[1] = 57; // To store field no. of valve in action 

    }
    /***************************/
    transmitStringToDebug("isFieldMoistureSensorWetLora_IN : ");
    transmitNumberToDebug(temporaryBytesArray, 2);
    transmitStringToDebug("\r\n");
    //********Debug log#end**************//
#endif

    do {
        sendCmdToLora(action, FieldNo);
    } while (loraAttempt < 2);
    if (LoraConnectionFailed || moistureSensorFailed) { // Unsuccessful Sensor reading
        moistureLevel = CLEAR;
        moistureSensorFailed = true;
    }
    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
    if ((moistureLevel / 100) >= fieldValve[FieldNo].wetValue) { //Field is full wet, no need to switch ON valve and motor, estimate new due dates
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isFieldMoistureSensorWetLora_Yes_Out\r\n");
        //********Debug log#end**************//
#endif
        return true;
    } else {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isFieldMoistureSensorWetLorat_No_Out\r\n");
        //********Debug log#end**************//
#endif
        return false;
    }

#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    /***************************/
    // for field no. 01 to 09
    if (FieldNo < 9) {
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
    }// for field no. 10 to 12
    else if (FieldNo > 8 && FieldNo < 12) {
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
    } else {
        temporaryBytesArray[0] = 57; // To store field no. of valve in action 
        temporaryBytesArray[1] = 57; // To store field no. of valve in action 

    }
    /***************************/
    transmitStringToDebug("isFieldMoistureSensorWetLora_OUT : ");
    transmitNumberToDebug(temporaryBytesArray, 2);
    transmitStringToDebug("\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Motor Dry run condition#Start********/

/*************************************************************************************************************************

This function is called to measure motor phase current to detect dry run condition.
The Dry run condition is measured in terms of voltage of CT connected to one of the phase of motor.
The motor phase current is high  and low  for Wet (load) and Dry (no load) condition respectively.
This leads the output of CT with high and low voltage.
For Dry condition, the CT voltage is high and for wet condition, the CT voltage is low.
Here ADC module is used to measure high/ low voltage of CT thereby identifying load/no load condition.

 **************************************************************************************************************************/

_Bool isMotorInNoLoad(void) {
    unsigned int ctOutput = 0;
    unsigned int temp = 0;
    lowPhaseCurrentDetected = false;
    dryRunDetected = false;
    temp = (fullLoadCutOff) / 10;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("isMotorInNoLoad_IN\r\n");
    //********Debug log#end**************//
#endif
    // Averaging measured pulse width
    selectChannel(CTchannel);
    ctOutput = getADCResult();
    if (ctOutput > temp && ctOutput <= noLoadCutOff) {
        dryRunDetected = true; //Set Low water level
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isMotorInNoLoad_Dry_Yes_OUT\r\n");
        //********Debug log#end**************//
#endif
        return true;
    } else if (ctOutput == 0 || (ctOutput > 0 && ctOutput <= temp)) { // no phase current
        lowPhaseCurrentDetected = true; //Set phase current low
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isMotorInNoLoad_LowPhase_Yes_OUT\r\n");
        //********Debug log#end**************//
#endif
        return true;
    } else {
        lowPhaseCurrentDetected = false;
        dryRunDetected = false; //Set High water level
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isMotorInNoLoad_Dry_LowPhase_No_OUT\r\n");
        //********Debug log#end**************//
#endif
        return false;
    }
}

/*********** Motor Dry run condition#End********/

/*********** Motor current calibration#Start********/

/*************************************************************************************************************************

This function is called to calibrate motor phase current to set no load, overload condition.
The Dry run condition is measured in terms of voltage of CT connected to one of the phase of motor.
The motor phase current is high  and low  for Wet (load) and Dry (no load) condition respectively.
This leads the output of CT with high and low voltage.
For Dry condition, the CT voltage is high and for wet condition, the CT voltage is low.
Here ADC module is used to measure high/ low voltage of CT thereby identifying load/no load condition.

 **************************************************************************************************************************/

void calibrateMotorCurrent(unsigned char loadType, unsigned char FieldNo) {
    unsigned int ctOutput = 0;
    unsigned char itr = 0, limit = 30;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("calibrateMotorCurrent_IN\r\n");
    //********Debug log#end**************//
#endif
    if (loadType == FullLoad) {
        switch (FieldNo) {
                __delay_ms(1000);
            case 0:
                Irri_Out1 = ON; // switch on valve for field 1
                break;
            case 1:
                Irri_Out2 = ON; // switch off valve for field 2
                break;
            case 2:
                Irri_Out3 = ON; // switch on valve for field 3
                break;
            case 3:
                Irri_Out4 = ON; // switch on valve for field 4
                break;
            case 4:
                Irri_Out5 = ON; // switch off valve for field 5
                break;
            case 5:
                Irri_Out6 = ON; // switch off valve for field 6
                break;
            case 6:
                Irri_Out7 = ON; // switch on valve for field 7
                break;
            case 7:
                Irri_Out8 = ON; // switch on valve for field 8
                break;
            case 8:
                Irri_Out9 = ON; // switch on valve for field 9
                break;
            case 9:
                Irri_Out10 = ON; // switch on valve for field 10
                break;
            case 10:
                Irri_Out11 = ON; // switch on valve for field 11
                break;
            case 11:
                Irri_Out12 = ON; // switch on valve for field 12
                break;
        }
    }
    if (Irri_Motor != ON) {
        __delay_ms(2500);
        __delay_ms(2500);
        Irri_Motor = ON;
        __delay_ms(100);
#ifdef STAR_DELTA_DEFINITIONS_H
        __delay_ms(500);
        Irri_MotorT = ON;
        __delay_ms(900);
        Irri_MotorT = OFF;
#endif
    }
    __delay_ms(2500);
    __delay_ms(2500);
    // Averaging measured pulse width
    //setBCDdigit(0x0F, 1); // BCD Indication for Flash
    __delay_ms(2500);
    __delay_ms(2500);
    selectChannel(CTchannel);
    if (loadType == NoLoad) {
        limit = 11; //~1.5 min
    }
    for (itr = 0; itr < limit; itr++) {
        ctOutput = getADCResult();
        __delay_ms(2500);
        lower8bits = ctOutput;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 1000) + 48);
        //setBCDdigit(temporaryBytesArray[0], 1);
        __delay_ms(1000);
        //setBCDdigit(0x0F, 1);
        __delay_ms(500);
        lower8bits = lower8bits % 1000;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
        //setBCDdigit(temporaryBytesArray[0], 1);
        __delay_ms(1000);
        //setBCDdigit(0x0F, 1);
        __delay_ms(500);
        lower8bits = lower8bits % 100;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 10) + 48);
        //setBCDdigit(temporaryBytesArray[0], 1);
        __delay_ms(1000);
        //setBCDdigit(0x0F, 1);
        __delay_ms(500);
        lower8bits = lower8bits % 10;
        temporaryBytesArray[0] = (unsigned char) (lower8bits + 48);
        //setBCDdigit(temporaryBytesArray[0], 1);
        __delay_ms(1000);
        //setBCDdigit(0x0F, 1);
        if (loadType == FullLoad) {
            fullLoadCutOff = ctOutput;
            noLoadCutOff = (7 * fullLoadCutOff) / 10;
        } else if (loadType == NoLoad) {
            noLoadCutOff = ctOutput;
        }
    }
    if (loadType == FullLoad) {
        Irri_Motor = OFF;
    }
    if (loadType == FullLoad) {
        __delay_ms(1000);
        switch (FieldNo) {
            case 0:
                Irri_Out1 = OFF; // switch on valve for field 1
                break;
            case 1:
                Irri_Out2 = OFF; // switch off valve for field 2
                break;
            case 2:
                Irri_Out3 = OFF; // switch on valve for field 3
                break;
            case 3:
                Irri_Out4 = OFF; // switch on valve for field 4
                break;
            case 4:
                Irri_Out5 = OFF; // switch off valve for field 5
                break;
            case 5:
                Irri_Out6 = OFF; // switch off valve for field 6
                break;
            case 6:
                Irri_Out7 = OFF; // switch on valve for field 7
                break;
            case 7:
                Irri_Out8 = OFF; // switch on valve for field 8
                break;
            case 8:
                Irri_Out9 = OFF; // switch on valve for field 9
                break;
            case 9:
                Irri_Out10 = OFF; // switch on valve for field 10
                break;
            case 10:
                Irri_Out11 = OFF; // switch on valve for field 11
                break;
            case 11:
                Irri_Out12 = OFF; // switch on valve for field 12
                break;
        }
    }
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("calibrateMotorCurrent_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Motor current calibration#End********/

/*********** DRY RUN Action#Start********/

/*************************************************************************************************************************

This function is called to perform actions after detecting dry run condition.
After detecting Dry run condition, stop all active valves and set all valves due from today to tomorrow.
Notify user about all actions

 **************************************************************************************************************************/

void doDryRunAction(void) {
    unsigned char field_No = CLEAR;
    unsigned int sleepCountVar = CLEAR;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("doDryRunAction_IN\r\n");
    //********Debug log#end**************//
#endif
    __delay_ms(100);
    fetchTimefromRTC(); // Get today's date
    __delay_ms(100);
    getDueDate(1); // calculate next day date										 
    for (field_No = 0; field_No < fieldCount; field_No++) {
        if (fieldValve[field_No].status == ON) {
            __delay_ms(100);
            powerOffMotor();
            __delay_ms(1000);
            deActivateValve(field_No); // Deactivate Valve upon Dry run condition and reset valve to next due time
            valveDue = false;
            fieldValve[field_No].status = OFF;
            fieldValve[field_No].cyclesExecuted = fieldValve[field_No].cycles;
            __delay_ms(100);
            saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
            __delay_ms(100);
            saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
            __delay_ms(100);
            sleepCountVar = readActiveSleepCountFromEeprom();
            if (fieldValve[field_No].isFertigationEnabled) {
                /************Fertigation switch off due to dry run***********/
                if (fieldValve[field_No].fertigationStage == injectPeriod) {
                    Fert_Motor = OFF; // Switch off fertigation valve in case it is ON
                    //Switch off all Injectors after completing fertigation on Period
                    Irri_Out9 = OFF;
                    Irri_Out10 = OFF;
                    Irri_Out11 = OFF;
                    Irri_Out12 = OFF;
                    fieldValve[field_No].fertigationStage = OFF;
                    fieldValve[field_No].fertigationValveInterrupted = true;
                    remainingFertigationOnPeriod = readActiveSleepCountFromEeprom();
                    __delay_ms(100);
                    saveRemainingFertigationOnPeriod();
                    __delay_ms(100);
                    saveFertigationValveStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    /******** Calculate and save Field Valve next date**********/
                    fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
                    fieldValve[field_No].nextDueMM = dueMM;
                    fieldValve[field_No].nextDueYY = dueYY;
                    __delay_ms(100);
                    saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    /***********************************************/
                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsDR1, userMobileNo, fieldNoRequired); // Acknowledge user about dry run detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                } else if (fieldValve[field_No].fertigationStage == wetPeriod) {
                    /******** Calculate and save Field Valve next date**********/
                    fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
                    fieldValve[field_No].nextDueMM = dueMM;
                    fieldValve[field_No].nextDueYY = dueYY;
                    __delay_ms(100);
                    saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    /***********************************************/
                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsDR2, userMobileNo, fieldNoRequired); // Acknowledge user about dry run detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                }
            } else {
                if (sleepCountVar > (fieldValve[field_No].onPeriod / 2)) { // major part of valve execution is pending hence shift to next day
                    /******** Calculate and save Field Valve next date**********/
                    fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
                    fieldValve[field_No].nextDueMM = dueMM;
                    fieldValve[field_No].nextDueYY = dueYY;
                    __delay_ms(100);
                    saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    /***********************************************/
                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsDR3, userMobileNo, fieldNoRequired); // Acknowledge user about dry run detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                } else { // next due date
                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsDR4, userMobileNo, fieldNoRequired); // Acknowledge user about dry run detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                }
            }
            if (Phase_Input) {
                /***************************/
                sendSms(SmsPh3, userMobileNo, noInfo); // Acknowledge user about Phase failure detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
            }
            else {
                /***************************/
                sendSms(SmsPh6, userMobileNo, noInfo); // Acknowledge user about Phase failure detected and action taken
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
            }

        } else if ((currentDD == fieldValve[field_No].nextDueDD && currentMM == fieldValve[field_No].nextDueMM && currentYY == fieldValve[field_No].nextDueYY)) {
            /******** Calculate and save Field Valve next date**********/
            fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
            fieldValve[field_No].nextDueMM = dueMM;
            fieldValve[field_No].nextDueYY = dueYY;
            fieldValve[field_No].cyclesExecuted = fieldValve[field_No].cycles;
            __delay_ms(100);
            saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
            __delay_ms(100);
            saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
            __delay_ms(100);
            /***********************************************/
        }
    }
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("doDryRunAction_OUT\r\n");
    //********Debug log#end**************//
#endif 
}

/*********** DRY RUN Action#End********/

/*********** Low Phase Action#Start********/

/*************************************************************************************************************************

This function is called to perform actions after detecting low phase condition.
After detecting low phase condition, stop all active valves and set them due when phase current recovers
Notify user about all actions

 **************************************************************************************************************************/

void doLowPhaseAction(void) {
    unsigned char field_No = CLEAR;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("dolowPhaseAction_IN\r\n");
    //********Debug log#end**************//
#endif
    /***************************/
    sendSms(SmsPh2, userMobileNo, noInfo); // Acknowledge user about low phase current
#ifdef SMS_DELIVERY_REPORT_ON_H
    sleepCount = 2; // Load sleep count for SMS transmission action
    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
    //setBCDdigit(0x05, 0);
    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
    /***************************/
    if (valveDue) {
        for (field_No = 0; field_No < fieldCount; field_No++) {
            if (fieldValve[field_No].status == ON) {
                powerOffMotor();
                __delay_ms(1000);
                deActivateValve(field_No); // Deactivate Valve upon phase failure condition and reset valve to next due time
                /************Fertigation switch off due to low phase detection***********/
                if (fieldValve[field_No].fertigationStage == injectPeriod) {
                    Fert_Motor = OFF; // Switch off fertigation valve in case it is ON
                    //Switch off all Injectors after completing fertigation on Period
                    Irri_Out9 = OFF;
                    Irri_Out10 = OFF;
                    Irri_Out11 = OFF;
                    Irri_Out12 = OFF;

                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsFert6, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation stopped action due to low phase detection
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider           
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                }
            }
        }
    }
    phaseFailureActionTaken = true;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("dolowPhaseAction_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Low Phase Action#End********/

/*********** Phase Failure Action#Start********/

/*************************************************************************************************************************

This function is called to perform actions after detecting phase failure condition.
After detecting phase failure condition, stop all active valves and set them due when phase recovers
Notify user about all actions

 **************************************************************************************************************************/

void doPhaseFailureAction(void) {
    unsigned char field_No = CLEAR;
#ifdef LCD_DISPLAY_ON_H
    lcdClear();
    lcdWriteStringAtCenter("Phase Failure", 1);
    lcdWriteStringAtCenter("Detected", 2);
#endif
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("doPhaseFailureAction_IN\r\n");
    //********Debug log#end**************//
#endif
    /***************************/
    sendSms(SmsPh1, userMobileNo, noInfo); // Acknowledge user about phase failure
#ifdef SMS_DELIVERY_REPORT_ON_H
    sleepCount = 2; // Load sleep count for SMS transmission action
    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
    //setBCDdigit(0x05, 0);
    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
    /***************************/
    if (valveDue) {
        for (field_No = 0; field_No < fieldCount; field_No++) {
            if (fieldValve[field_No].status == ON) {
                powerOffMotor();
                __delay_ms(1000);
                deActivateValve(field_No); // Deactivate Valve upon phase failure condition and reset valve to next due time
                /************Fertigation switch off due to Phase failure***********/
                if (fieldValve[field_No].fertigationStage == injectPeriod) {
                    Fert_Motor = OFF; // Switch off fertigation valve in case it is ON
                    //Switch off all Injectors after completing fertigation on Period
                    Irri_Out9 = OFF;
                    Irri_Out10 = OFF;
                    Irri_Out11 = OFF;
                    Irri_Out12 = OFF;

                    /***************************/
                    // for field no. 01 to 09
                    if (field_No < 9) {
                        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                    }// for field no. 10 to 12
                    else if (field_No > 8 && field_No < 12) {
                        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                        temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                    }
                    /***************************/

                    /***************************/
                    sendSms(SmsFert6, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation stopped action due to PhaseFailure
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider           
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                }
            }
        }
    }
    phaseFailureActionTaken = true;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("doPhaseFailureAction_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Phase Failure Action#End********/

/*********** RTC battery drained#Start********/

/*************************************************************************************************************************

This function is called to measure motor phase current to detect dry run condition.
The Dry run condition is measured in terms of voltage of CT connected to one of the phase of motor.
The motor phase current is high  and low  for Wet (load) and Dry (no load) condition respectively.
This leads the output of CT with high and low voltage.
For Dry condition, the CT voltage is high and for wet condition, the CT voltage is low.
Here ADC module is used to measure high/ low voltage of CT thereby identifying load/no load condition.

 **************************************************************************************************************************/

_Bool isRTCBatteryDrained(void) {
    unsigned int batteryVoltage = 0;
    unsigned int batteryVoltageCutoff = 555; //~2.7 v
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("isRTCBatteryDrained_IN\r\n");
    //********Debug log#end**************//
#endif
	rtcBatteryLevelChecked = true;							  
    selectChannel(RTCchannel);
    RTC_Trigger = ENABLED;
    __delay_ms(50);
    batteryVoltage = getADCResult();
    RTC_Trigger = DISABLED;
    if (batteryVoltage <= batteryVoltageCutoff) {
        lowRTCBatteryDetected = true;
        __delay_ms(100);
        saveRTCBatteryStatus();
        __delay_ms(100);
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isRTCBatteryDrained_Yes_OUT\r\n");
        //********Debug log#end**************//
#endif
        return true;
    } else {
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("isRTCBatteryDrained_No_OUT\r\n");
        //********Debug log#end**************//
#endif
        return false;
    }
}

/*********** RTC battery drained#End********/


/*********** Check RYB phase Detection#Start********/

/*************************************************************************************************************************

This function is called to detect phase failure condition.
The phase failure condition is measured by scanning three phase input lines.
Each phase line is converted into TTL signal using Digital signal comparator.high  and low for lost (out) and present (In) condition respectively.
The output of Comparator is high  and low for lost (out) and present (In) condition respectively.
For Phase failure condition, at least one comparator output is high and for Phase detected condition all comparator output is low.
Here Rising Edge and Falling Edge is used to detect high/ low Comparator Output.

 **************************************************************************************************************************/

_Bool phaseFailure(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("phaseFailure_IN\r\n");
    //********Debug log#end**************//
#endif
    if (!Phase_Input) { //All 3 phases are ON
        phaseFailureDetected = false;
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("All Power Phase", 1);
        lcdWriteStringAtCenter("Detected", 2);
        __delay_ms(3000);
#endif
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("phaseFailure_No_OUT\r\n");
        //********Debug log#end**************//
#endif
        return false; //no Phase failure
    } else { // one phase is lost
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("Phase Failure", 1);
        lcdWriteStringAtCenter("Detected", 2);
        __delay_ms(3000);
#endif
        phaseFailureDetected = true; //true
        phaseFailureActionTaken = false;
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("phaseFailure_Yes_OUT\r\n");
        //********Debug log#end**************//
#endif
        return true; //phase failure // true
    }
}

/*********** Motor Dry run condition#End********/


/*********** Motor Power On #Start********/

/*************************************************************************************************************************

This function is called to power on motor
The purpose of this function is to activate relays to Switch ON Motor
 *
 **************************************************************************************************************************/
void powerOnMotor(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("powerOnMotor_IN\r\n");
    //********Debug log#end**************//
#endif
    __delay_ms(100);
    Irri_Motor = ON;
    Timer0Overflow = 0;
    T0CON0bits.T0EN = ON; // Start timer0 to initiate 1 min cycle
    if (filtrationEnabled) {
        filtrationCycleSequence = 1;
    } else {
        filtrationCycleSequence = 99;
    }
    dryRunCheckCount = 0;
#ifdef STAR_DELTA_DEFINITIONS_H
    __delay_ms(500);
    Irri_MotorT = ON;
    __delay_ms(900);
    Irri_MotorT = OFF;
#endif

#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("powerOnMotor_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Motor Power On#End********/


/*********** Motor Power Off #Start********/

/*************************************************************************************************************************

This function is called to power Off motor
The purpose of this function is de-activate relays to Switch OFF Motor

 **************************************************************************************************************************/

void powerOffMotor(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("powerOffMotor_IN\r\n");
    //********Debug log#end**************//
#endif
    T0CON0bits.T0EN = OFF; // stop timer0
    __delay_ms(100);
    Filt_Out1 = OFF; // switch off filtration  valve if it is ON
    __delay_ms(50);
    Filt_Out2 = OFF; // switch off filtration  valve if it is ON
    __delay_ms(50);
    Filt_Out3 = OFF; // switch off filtration  valve if it is ON
    __delay_ms(50);
    Irri_Motor = OFF; // switch off Motor
    __delay_ms(50);

#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("powerOffMotor_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*********** Motor Power Off#End********/


/*********** Field Valve Activation#Start********/

/*************************************************************************************************************************

This function is called to activate valve
The purpose of this function is to activate mentioned field valve and notify user about activation through SMS
	
 **************************************************************************************************************************/
void activateValve(unsigned char FieldNo) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("sendActivateValveCmdToLora_IN\r\n");
    //********Debug log#end**************//
#endif
    unsigned char action;
    loraAttempt = 0;
    action = 0x00; // activate valve action
    do {
        sendCmdToLora(action, FieldNo);
    } while (loraAttempt < 2);
    if (!LoraConnectionFailed && loraAttempt == 99) { // Successful Valve Activation
        // check field no. of valve in action
        fieldValve[FieldNo].status = ON; //notify field valve status
        valveDue = true; // Set Valve ON status
        loraAliveCount = CLEAR;
        loraAliveCountCheck = CLEAR;
        __delay_ms(100);
        saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[FieldNo], &fieldValve[FieldNo]);
        __delay_ms(100);
        /***************************/
        // for field no. 01 to 09
        if (FieldNo < 9) {
            temporaryBytesArray[0] = 48; // To store field no. of valve in action 
            temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
        }// for field no. 10 to 12
        else if (FieldNo > 8 && FieldNo < 12) {
            temporaryBytesArray[0] = 49; // To store field no. of valve in action 
            temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
        }
        /***************************/
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("Valve: ");
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\n");
        //********Debug log#end**************//
#endif

        if (moistureSensorFailed) {
            moistureSensorFailed = false;
            
            /***************************/
            sendSms(SmsMS1, userMobileNo, fieldNoRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            //setBCDdigit(0x05, 0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
            /***************************/
#ifdef LCD_DISPLAY_ON_H             
            lcdClear();
            lcdWriteStringAtCenter("Irrigation Started", 1);
            lcdWriteStringAtCenter("With Sensor Failure", 2);
            lcdWriteStringAtCenter("For Field No:", 3);
            sprintf(temporaryBytesArray,"%d",FieldNo+1);
            lcdWriteStringAtCenter(temporaryBytesArray, 4);
            __delay_ms(2000);
#endif
        } else {
            /***************************/
            sendSms(SmsIrr4, userMobileNo, fieldNoRequired); // Acknowledge user about successful Irrigation started action
#ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            //setBCDdigit(0x05, 0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
            /***************************/
#ifdef LCD_DISPLAY_ON_H             
            lcdClear();
            lcdWriteStringAtCenter("Irrigation Started", 1);
            lcdWriteStringAtCenter("Successfully", 2);
            lcdWriteStringAtCenter("For Field No:", 3);
            sprintf(temporaryBytesArray,"%d",FieldNo+1);
            lcdWriteStringAtCenter(temporaryBytesArray, 4);
            __delay_ms(2000);
#endif
        }
    } else { //Skip current valve execution and go for next
        valveDue = false;
        fieldValve[FieldNo].status = OFF;
        fieldValve[FieldNo].cyclesExecuted = fieldValve[FieldNo].cycles;
        startFieldNo = FieldNo + 1; // scan for next field no.
        __delay_ms(100);
        getDueDate(fieldValve[FieldNo].offPeriod); // calculate next due date of valve
        __delay_ms(100);
        fieldValve[FieldNo].nextDueDD = (unsigned char) dueDD;
        fieldValve[FieldNo].nextDueMM = dueMM;
        fieldValve[FieldNo].nextDueYY = dueYY;
        __delay_ms(100);
        saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[FieldNo], &fieldValve[FieldNo]);
        __delay_ms(100);
        saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[FieldNo], &fieldValve[FieldNo]);
        __delay_ms(100);
        saveIrrigationValveDueTimeIntoEeprom(eepromAddress[FieldNo], &fieldValve[FieldNo]);
        __delay_ms(100);

        /***************************/
        // for field no. 01 to 09
        if (FieldNo < 9) {
            temporaryBytesArray[0] = 48; // To store field no. of valve in action 
            temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
        }// for field no. 10 to 12
        else if (FieldNo > 8 && FieldNo < 12) {
            temporaryBytesArray[0] = 49; // To store field no. of valve in action 
            temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
        }
        /***************************/

        /***************************/
        sendSms(SmsIrr8, userMobileNo, fieldNoRequired); // Acknowledge user about Irrigation not started due to Lora connection failure						
#ifdef SMS_DELIVERY_REPORT_ON_H
        sleepCount = 2; // Load sleep count for SMS transmission action
        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
        //setBCDdigit(0x05, 0);
        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
        /***************************/
#ifdef LCD_DISPLAY_ON_H 
        lcdClear();
        lcdWriteStringAtCenter("Irrigation Skipped", 1);
        lcdWriteStringAtCenter("With No Response",2);
        lcdWriteStringAtCenter("From Field No:",3);
        sprintf(temporaryBytesArray,"%d",FieldNo+1);
        lcdWriteStringAtCenter(temporaryBytesArray, 4);
        __delay_ms(2000);
#endif
    }
#ifdef DEBUG_MODE_ON_H    
    //********Debug log#start************//
    transmitStringToDebug("activateValve_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/*********** Field Valve Activation#End********/


/*********** Field Valve De-Activation#Start********/

/*************************************************************************************************************************

This function is called to de-activate valve
The purpose of this function is to deactivate mentioned field valve and notify user about De-activation through SMS

 **************************************************************************************************************************/
void deActivateValve(unsigned char FieldNo) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deActivateValve_IN for valve: ");
    //********Debug log#end**************//
#endif
    // check field no. of valve in action
    unsigned char action;
    loraAttempt = 0;
    action = 0x01;
    do {
        sendCmdToLora(action, FieldNo);
    } while (loraAttempt < 2);
    /***************************/
    // for field no. 01 to 09
    if (FieldNo < 9) {
        temporaryBytesArray[0] = 48; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 49; // To store field no. of valve in action 
    }// for field no. 10 to 12
    else if (FieldNo > 8 && FieldNo < 12) {
        temporaryBytesArray[0] = 49; // To store field no. of valve in action 
        temporaryBytesArray[1] = FieldNo + 39; // To store field no. of valve in action 
    }
    /***************************/
    if (!LoraConnectionFailed && loraAttempt == 99) { // Successful Valve DeActivation

        /***************************/
#ifdef DEBUG_MODE_ON_H
        //********Debug log#start************//
        transmitStringToDebug("Valve: ");
        transmitNumberToDebug(temporaryBytesArray, 2);
        transmitStringToDebug("\r\n");
        //********Debug log#end**************//
#endif
        /***************************/
        sendSms(SmsIrr5, userMobileNo, fieldNoRequired); // Acknowledge user about successful Irrigation stopped action
#ifdef SMS_DELIVERY_REPORT_ON_H
        sleepCount = 2; // Load sleep count for SMS transmission action
        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
        //setBCDdigit(0x05, 0);
        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
        /***************************/
#ifdef LCD_DISPLAY_ON_H        
        lcdClear();
        lcdWriteStringAtCenter("Irrigation Stopped", 1);
        lcdWriteStringAtCenter("Successfully",2);
        lcdWriteStringAtCenter("For Field No:",3);
        sprintf(temporaryBytesArray,"%d",FieldNo+1);
        lcdWriteStringAtCenter(temporaryBytesArray, 4);
        __delay_ms(2000);
#endif        
    } else {
        /***************************/
        sendSms(SmsIrr9, userMobileNo, fieldNoRequired); // Acknowledge user about Irrigation stopped with Lora connection failure						
#ifdef SMS_DELIVERY_REPORT_ON_H
        sleepCount = 2; // Load sleep count for SMS transmission action
        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
        //setBCDdigit(0x05, 0);
        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
        /***************************/
#ifdef LCD_DISPLAY_ON_H        
        lcdClear();
        lcdWriteStringAtCenter("Irrigation Stopped", 1);
        lcdWriteStringAtCenter("With No Response",2);
        lcdWriteStringAtCenter("From Field No:",3);
        sprintf(temporaryBytesArray,"%d",FieldNo+1);
        lcdWriteStringAtCenter(temporaryBytesArray, 4);
        __delay_ms(2000);
#endif        
    }
#ifdef DEBUG_MODE_ON_H    
    //********Debug log#start************//
    transmitStringToDebug("deActivateValve_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/*********** Field Valve De-Activation#End********/


/********************Deep Sleep function#Start************************/

/*************************************************************************************************************************

This function is called to activate deep sleep mode
The purpose of this function is to go into sleep mode until it is interrupted by GSM or Sleep count is reached to 0

 **************************************************************************************************************************/
void deepSleep(void) {
    // check until sleep timer ends given sleep count
    while (sleepCount > 0 && !newSMSRcvd) {
        if (phaseFailureDetected) {
#ifdef LCD_DISPLAY_ON_H   
            lcdClear();
            lcdWriteStringAtCenter("Phase Failure", 1);
            lcdWriteStringAtCenter("Detected", 2);
#endif
            if (!phaseFailureActionTaken) {
                doPhaseFailureAction();
            }
            sleepCount = 65500;
            //setBCDdigit(0x03, 0); // (3.) BCD Indication for Phase Failure Error
        } else if (Irri_Motor == ON) { // Motor is ON without any external/Internal interrupt
            saveActiveSleepCountIntoEeprom(); // Save current valve on time
            // check Motor Dry run condition after each sleep count
            if (dryRunCheckCount > 2) {
                if (isMotorInNoLoad()) {							
					if (dryRunDetected) {
						doDryRunAction();
					} else if (lowPhaseCurrentDetected) {
						doLowPhaseAction();
						sleepCount = 65500; // undefined sleep until phase comes back
					}
				}
            } else {
#ifdef LCD_DISPLAY_ON_H 
                lcdClear();
                lcdWriteStringAtCenter("Irrigation Running", 1);
                lcdWriteStringAtCenter("For Field No: ", 2);
                //sprintf(temporaryBytesArray,"%d",FieldNo+1);
                lcdWriteStringAtCenter(temporaryBytesArray, 4);
                //setBCDdigit(0x0C, 1); // (u) BCD Indication for valve in action
#endif     
            }
        } else if (dryRunDetected) {
#ifdef LCD_DISPLAY_ON_H 
            lcdClear();
            lcdWriteStringAtCenter("Dry Run", 1);
            lcdWriteStringAtCenter("Detected", 2);
            //setBCDdigit(0x0C, 0); // (u.) BCD Indication for Dry Run Detected Error
#endif
        } else if (lowPhaseCurrentDetected) {
#ifdef LCD_DISPLAY_ON_H 
            lcdClear();
            lcdWriteStringAtCenter("Found Low Phase", 1);
            lcdWriteStringAtCenter("Current", 2);
            //setBCDdigit(0x03, 0); // (3.) BCD Indication for Phase Failure Error
#endif
        } else if (lowRTCBatteryDetected) {
#ifdef LCD_DISPLAY_ON_H 
            lcdClear();
            lcdWriteStringAtCenter("Found Low RTC", 1);
            lcdWriteStringAtCenter("Battery", 2);
            //setBCDdigit(0x02, 0); // (2.) BCD Indication for RTC Battery Low Error
#endif
        } else if (systemAuthenticated) {
            //setBCDdigit(0x01, 1); // (1) BCD Indication for System Authenticated
        }
        Run_led = DARK; // Led Indication for system in Sleep/ Idle Mode
        inSleepMode = true; // Indicate in Sleep mode
        WDTCON0bits.SWDTEN = ENABLED; // Enable sleep mode timer
        if (sleepCount > 0 && !newSMSRcvd) {
            Sleep(); // CPU sleep. Wakeup when Watchdog overflows, each of 16 Seconds if value of WDTPS is 4096
        }
        if (valveDue) {
            __delay_ms(1500); // compensate for new sms when valve is active
        }
        WDTCON0bits.SWDTEN = DISABLED; //turn off sleep mode timer
        Run_led = GLOW; // Led Indication for system in Operational Mode
        if (!valveDue && !phaseFailureDetected && !lowPhaseCurrentDetected) {
            sleepCount--; // Decrement sleep count after every sleep cycle
        }
    }
    if (sleepCount == 0 && !newSMSRcvd) {
        __delay_ms(2000); // To compensate incoming SMS if valve is due within 10 minutes
    }
    inSleepMode = false; // Indicate not in sleep mode

}
/********************Deep Sleep function#End************************/


/*************Initialize#Start**********/

/*************************************************************************************************************************

This function is called to initialized system
The purpose of this function is to define port lines, interrupt priorities and to configure Timer and UART module.

 **************************************************************************************************************************/
void configureController(void) {

    BSR = 0x0f; // Set BSR for Banked SFR
    LATA = 0x00; // Set all output bits to zero for PORTA
    TRISA = 0b11100000; // Set RA<0:4> --Output~MUX Control lines, RA<5> --Input~ <Mux i/p>, RB<6:7> --Inputs~ <OSC1-2>
    ANSELA = 0b00100000; // RA<0:4,6:7> --Digital, RA<5> -- Analog
    WPUA = 0x00; //Weak Pull-up disabled
    ODCONA = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONA = 0xFF; //Port pin slew rate is limited
    INLVLA = 0xFF; //ST input used for port reads and interrupt-on-change

    LATB = 0x00; // Set all output bits to zero for PORTB
    TRISB = 0b11000111; // Set RB<0> --input~3ph interrupt, RB<1:2> --input~ <Temp,windspeed sensor>, RB<3:5> --outputs~ <Filtration1-3>,  RB<6:7> --input~ <PH,EC sensor>
    ANSELB = 0b11000110; // RB<0,3:5> -- Digital, RB<1:2,6:7> --Analog
    WPUB = 0x00; //Weak Pull-up disabled
    ODCONB = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONB = 0xFF; //Port pin slew rate is limited
    INLVLB = 0xFF; //ST input used for port reads and interrupt-on-change
    IOCBN = 0b00000001; //Interrupt-on-Change Negative Edge Enable bits for RB0 --RYB phase
    IOCBP = 0b00000001; //Interrupt-on-Change Positive Edge Enable bits for RB0 --RYB phase
    IOCBF = 0b00000000; // Clear all initial IOC flags
    PIE0bits.IOCIE = ENABLED; //Peripheral Interrupt-on-Change Enabled

    LATC = 0x00; // Set all output bits to zero for PORTC
    TRISC = 0b10111000; // RC<0:2> --Outputs~<Filtration4:6>, RC<3:4> --Output~<SCL,SDA(LCD)>, RC<5> --Input~<Moisture sensor>, RC<6> --Output~TX1 (LORA), RC<7> --Input~ RX1 (LORA)
    WPUC = 0b00011000; ///* Set pull-up resistors for RC3 and RC4 */
    ODCONC = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONC = 0xFF; //Port pin slew rate is limited
    INLVLC = 0xFF; //ST input used for port reads and interrupt-on-change

    LATD = 0x00; // Set all output bits to zero for PORTD
    TRISD = 0b01100011; // RD<0:1> --Input~ <CT,RTCbattery>, RD<2:4,7> --Outputs~<IrrControlLines1:3,4>,  RD<5:6> --Outputs~ <SCL,SDA(RTC)>
    ANSELD = 0b00000011; // RD<0:1> --Analog , RD<2:7> --Digital 
    WPUD = 0x00; //Weak Pull-up disabled
    ODCOND = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCOND = 0xFF; //Port pin slew rate is limited
    INLVLD = 0xFF; //ST input used for port reads and interrupt-on-change

    LATE = 0x00; // Set all output bits to zero for PORTE
    TRISE = 0b00001110; // RE<0> --Output~ <TX3(GSM)>, RE<1> --Input~ <RX3(GSM)>, RE<2:3> --Input~ <Water,Fertflowsensor>, RE<4:6> --Output~ <FertInj1:3>, RE<7> --Output~ <IrrControlLine5>
    ANSELE = 0b00000000; // RE<0:7> --Digital
    WPUE = 0x00; //Weak Pull-up disabled //Weak Pull-up disabled
    ODCONE = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONE = 0xFF; //Port pin slew rate is limited
    INLVLE = 0xFF; //ST input used for port reads and interrupt-on-change
    IOCEN = 0b00001100; //Interrupt-on-Change Negative Edge Enable bits for RE2 and RE3 --Irrigation and Fertigation Flow Sensor
    IOCEP = 0b00001100; //Interrupt-on-Change Positive Edge Enable bits for RE2 and RE3 --Irrigation and Fertigation Flow Sensor
    IOCEF = 0b00000000; // Clear all initial IOC flags
    PIE0bits.IOCIE = ENABLED; //Peripheral Interrupt-on-Change Enabled

    LATF = 0x00; // Set all output bits to zero for PORTF
    TRISF = 0x00; // RF<0:7> --Output~<IrrControlLines6:13>
    ANSELF = 0x00; // RF<0:7> --Digital 
    WPUF = 0x00; //Weak Pull-up disabled
    ODCONF = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONF = 0xFF; //Port pin slew rate is limited
    INLVLF = 0xFF; //ST input used for port reads and interrupt-on-change

    LATG = 0x00; // Set all output bits to zero for PORTG
    TRISG = 0b00100010; // RG<0,3> --Outputs~ <RTC trg, FertInj4>, RG<1:2> --Inputs~ <Prssure sensor1:2>, RG<4,6:7> --Outputs~ <IrrControlLines14:16>, RG<5> --Input~ MCLR
    ANSELG = 0b00000110; // RG<0,3:7> --Digital, RG<1:2> --Analog
    WPUG = 0x00; //Weak Pull-up disabled
    ODCONG = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONG = 0xFF; //Port pin slew rate is limited
    INLVLG = 0xFF; //ST input used for port reads and interrupt-on-change

    LATH = 0x00; // Set all output bits to zero for PORTH
    TRISH = 0b00000000; // Set RH<0:3> as outputs: RH<0> -> MotorPin1, RH<1> -> MotorPin2, RH<2> -> Fertigation Motor, RH<3> -> RUN LED
    WPUH = 0x00; //Weak Pull-up disabled
    ODCONH = 0x00; //Output drives both high-going and low-going signals (source and sink current)
    SLRCONH = 0xFF; //Port pin slew rate is limited
    INLVLH = 0xFF; //ST input used for port reads and interrupt-on-change


    //-----------ADC_Config-----------------------//

    ADREF = 0b00000000; // Reference voltage set to VDD and GND
    ADCON1 = 0X00;
    ADCON2 = 0X00;
    ADCON3 = 0X00;
    ADACQ = 0X00;
    ADCAP = 0X00;
    ADRPT = 0X00;
    ADACT = 0X00;
    ADCON0bits.ADFM = 1; // ADC results Format -- ADRES and ADPREV data are right-justified
    ADCON0bits.ADCS = 1; // ADC Clock supplied from FRC dedicated oscillator
    ADCON0bits.ADON = 1; //ADC is enabled
    ADCON0bits.ADCONT = 0; //ADC Continuous Operation is disabled

    //-----------Timer0_Config (60 sec) used for SLEEP Count control during Motor ON period and to control filtration  cycle sequence followup----------------------//
    //-----------Timer will not halt in sleep mode------------------------------------------------------//

    T0CON0 = 0b00010000; // 16 bit Timer 
    T0CON1 = 0b10011000; // Asynchronous with LFINTOSC 31KHZ as clock source with prescalar 1:256
    TMR0H = 0xE3; // Load Timer0 Register Higher Byte 0xE390
    TMR0L = 0xB0; // Load Timer0 Register Lower Byte FFFF-(60*31K)/(256) = 0xE39F)
    PIR0bits.TMR0IF = CLEAR; // Clear Timer0 Overflow Interrupt at start
    PIE0bits.TMR0IE = ENABLED; // Enables the Timer0 Overflow Interrupt
    IPR0bits.TMR0IP = LOW; // Low Timer0 Overflow Interrupt Priority

    //-----------Timer1_Config used for calculation pulse width of moisture sensor output-----------//
    //-----------Timer will halt in sleep mode------------------------------------------------------//

    T1CON = 0b00000010; // 16 bit Timer with Synchronous mode
    TMR1H = CLEAR; // Clear Timer1 Register Higher Byte
    TMR1L = CLEAR; // Clear Timer1 Register Lower Byte
    TMR1CLK = 0b00000001; //  Clock source as FOSC/4
    PIR5bits.TMR1IF = CLEAR; // Clear Timer1 Overflow Interrupt at start
    PIE5bits.TMR1IE = ENABLED; // Enables the Timer1 Overflow Interrupt
    IPR5bits.TMR1IP = LOW; // Low Timer1 Overflow Interrupt Priority

    //-----------Config Timer2_Config used for PWM1,2,3,4 output-----------//
    //-----------Timer will halt in sleep mode------------------------------------------------------//

    /* TIMER2 clock source is FOSC/4 */
    T2CLKCONbits.CS = 1;
    /* TIMER2 counter reset */
    T2TMR = 0x00;
    /* TIMER2 ON, prescaler 1:32, postscaler 1:1 */
    T2CONbits.OUTPS = 0;
    T2CONbits.CKPS = 5; // 5--1:32  4--1:16  3--1:8  2--1:4 1--1:2 0--1:1
    T2CONbits.T2ON = 1;
    /* Configure the default 2 kHZ*/
    T2PR = 249;

    /* Configure PWM1 START */
    /* Configure CCP pin*/
    RG1PPS = 0X05; // RG1 PWM output
    /* MODE PWM; EN enabled; FMT left_aligned */
    CCP1CONbits.MODE = 0x0C;
    CCP1CONbits.FMT = 1;
    CCP1CONbits.EN = 1;
    /* Selecting Timer 2 */
    CCPTMRS0bits.C1TSEL = 0;
    /* Configure the default 10 %duty cycle */
    CCPR1 = 6336;
    /* Configure PWM1 End */

    /* Configure PWM2 START */
    /* Configure CCP pin*/
    RG2PPS = 0X06; // RG2 PWM output
    /* MODE PWM; EN enabled; FMT left_aligned */
    CCP2CONbits.MODE = 0x0C;
    CCP2CONbits.FMT = 1;
    CCP2CONbits.EN = 1;
    /* Selecting Timer 2 */
    CCPTMRS0bits.C2TSEL = 0;
    /* Configure the default 10 %duty cycle */
    CCPR2 = 6336;
    /* Configure PWM2 End */

    /* Configure PWM3 START */
    /* Configure CCP pin*/
    RG3PPS = 0X07; // RG3 PWM output
    /* MODE PWM; EN enabled; FMT left_aligned */
    CCP3CONbits.MODE = 0x0C;
    CCP3CONbits.FMT = 1;
    CCP3CONbits.EN = 1;
    /* Selecting Timer 2 */
    CCPTMRS0bits.C3TSEL = 0;
    /* Configure the default 10 %duty cycle */
    CCPR3 = 6336;
    /* Configure PWM3 End */

    /* Configure PWM4 START */
    /* MODE PWM; EN enabled; FMT left_aligned */
    CCP4CONbits.MODE = 0x0C;
    CCP4CONbits.FMT = 1;
    CCP4CONbits.EN = 1;
    /* Selecting Timer 2 */
    CCPTMRS0bits.C4TSEL = 0;
    /* Configure the default 10 %duty cycle */
    CCPR4 = 6336;
    /* Configure PWM4 End */

    //-----------Timer3_Config (1 sec) used if command fails to respond within timer limit----------------------//
    //-----------Timer will halt in sleep mode------------------------------------------------------//

    T3CON = 0b00110010; // 16 bit Timer with synchronous mode with 1:8 pre scale
    TMR3CLK = 0b00000100; // Clock source as LFINTOSC
    TMR3H = 0xF0; // Load Timer3 Register Higher Byte 
    TMR3L = 0xDC; // Load Timer3 Register lower Byte 
    PIR5bits.TMR3IF = CLEAR; // Clear Timer3 Overflow Interrupt at start
    PIE5bits.TMR3IE = ENABLED; // Enables the Timer3 Overflow Interrupt
    IPR5bits.TMR3IP = LOW; // Low Timer3 Overflow Interrupt Priority

    //-----------UART1_Config PRODUCTION LORA-----------------------//
    TX1STA = 0b00100100; // 8 Bit Transmission Enabled with High Baud Rate
    RC1STA = 0b10010000; // 8 Bit Reception Enabled with Continuous Reception
    SP1BRG = 0x0681; // XTAL=16MHz, Fosc=64Mhz for SYNC=0 BRGH=1 BRG16=1 (Asynchronous high 16 bit baud rate)
    RC7PPS = 0x17; //EUSART1 Receive
    RC6PPS = 0x0C; //EUSART1 Transmit
    temp = RC1REG; // Empty buffer
    BAUD1CON = 0b00001000; // 16 Bit Baud Rate Register used
    PIE3bits.RC1IE = ENABLED; // Enables the EUSART Receive Interrupt
    PIE3bits.TX1IE = DISABLED; // Disables the EUSART Transmit Interrupt
    IPR3bits.RC1IP = HIGH; // EUSART Receive Interrupt Priority


    //-----------UART3_Config PRODUCTION GSM/GPRS-----------------------//

    TX3STA = 0b00100100; // 8 Bit Transmission Enabled with High Baud Rate
    RC3STA = 0b10010000; // 8 Bit Reception Enabled with Continuous Reception
    SP3BRG = 0x0681; // XTAL=16MHz, Fosc=64Mhz for SYNC=0 BRGH=1 BRG16=1 (Asynchronous high 16 bit baud rate)
    RE1PPS = 0x21; //EUSART3 Receive
    RE0PPS = 0x10; //EUSART3 Transmit
    temp = RC3REG; // Empty buffer
    BAUD3CON = 0b00001000; // 16 Bit Baud Rate Register used
    PIE4bits.RC3IE = ENABLED; // Enables the EUSART Receive Interrupt
    PIE4bits.TX3IE = DISABLED; // Disables the EUSART Transmit Interrupt
    IPR4bits.RC3IP = HIGH; // EUSART Receive Interrupt Priority

    //-----------I2C_Config_RTC-----------------------//

    SSP2STAT |= 0x80; //Slew Rate Disabled
    SSP2CON1 = 0b00101000; //Master mode 0x28
    SSP2DATPPS = 0x1D; //RD5<-MSSP2:SDA2;  
    SSP2CLKPPS = 0x1E; //RD6<-MSSP2:SCL2;
    RD5PPS = 0x1C; //RD5->MSSP2:SDA2;    
    RD6PPS = 0x1B; //RD6->MSSP2:SCL2;    
    SSP2ADD = 119; //DS1307 I2C address

    //-----------I2C_Config_LCD-----------------------//

    SSP1STAT |= 0x80; //Slew Rate Disabled
    SSP1CON1 = 0b00101000; //Master mode 0x28
    SSP1CON2 = 0x00;
    SSP1STAT = 0x00;
    /* PPS setting for using RC3 as SCL */
    SSP1CLKPPS = 0x13; //RC3<-MSSP2:SCL1;
    RC3PPS = 0x19; //RC3->MSSP2:SCL1; 

    /* PPS setting for using RC4 as SDA */
    SSP1DATPPS = 0x14; //RD5<-MSSP2:SDA1;   
    RC4PPS = 0x1A; //RC4->MSSP2:SDA1;    

    SSP1ADD = ((_XTAL_FREQ / 4) / I2C_BAUDRATE - 1); //For 100K Hz it is 0x9F

    //-----------Interrupt_Config---------------//

    OSCENbits.SOSCEN = DISABLED; //Secondary Oscillator is disabled
    INTCONbits.IPEN = ENABLED; // Enables Priority Levels on Interrupts
    INTCONbits.PEIE = ENABLED; // Enables all unmasked  peripheral interrupts
    INTCONbits.GIE = ENABLED; // Enables all unmasked Global interrupts
    CPUDOZEbits.IDLEN = ENABLED; //Device enters into Idle mode on SLEEP Instruction.
#ifdef LCD_DISPLAY_ON_H    
    lcdInit();
    lcdClear();
    lcdWriteStringAtCenter("Bhoomi Jalsandharan", 2);
    lcdWriteStringAtCenter("Udyami LLP", 3);
    __delay_ms(3000);
    __delay_ms(3000);
    lcdClear();
    lcdWriteStringAtCenter("WireLess Irrigation", 2);
    lcdWriteStringAtCenter("Control System", 3);
    __delay_ms(3000);
#endif
}
/*************Initialize#End**********/

/*************setFactoryPincode#Start**********/

/*************************************************************************************************************************

This function is called to set Factory pincode on loading program for first time
The purpose of this function is to generate and store one time factory pincode

 ***************************************************************************************************************************/
void setFactoryPincode(void) {
    readDeviceProgramStatusFromEeprom();
    __delay_ms(50);
    if (DeviceBurnStatus == false) {
        DeviceBurnStatus = true;
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("Setting", 1);
        lcdWriteStringAtCenter("Factory PinCode", 2);
#endif
        randomPasswordGeneration();
        lcdWriteStringAtCenter(factryPswrd, 3);
        __delay_ms(3000);
        saveFactryPswrdIntoEeprom();
        saveDeviceProgramStatusIntoEeprom();
    }
}
/*************setFactoryPincode#End**********/

/*************checkResetType#Start**********/

/*************************************************************************************************************************

This function is called to check reset type on reset interrupt
The purpose of this function is to check which type of reset occurred

 ***************************************************************************************************************************/

unsigned char checkResetType(void) {
    unsigned char resetType = 0;
    // check if system power reset occurred
    if (!PCON0bits.nPOR || !PCON0bits.nRI || !PCON0bits.nRMCLR || !PCON0bits.nBOR || !PCON0bits.nRWDT || PCON0bits.STKOVF || PCON0bits.STKUNF) {
        if (!PCON0bits.nPOR || !PCON0bits.nBOR) {
            PCON0bits.nRMCLR = SET; // Reset reset on MCLR status
            PCON0bits.nRI = SET; // Reset reset on instruction status
            PCON0bits.nRWDT = SET; // Reset WDT status
            PCON0bits.STKOVF = CLEAR; // Reset Stack Overflow status
            PCON0bits.STKUNF = CLEAR; // Reset Stack underflow status
        }
        if (!PCON0bits.nPOR) {
            resetType = PowerOnReset;
            PCON0bits.nPOR = SET; // Reset power status
        } else if (!PCON0bits.nBOR) {
            resetType = LowPowerReset;
            PCON0bits.nBOR = SET; // Reset BOR status
        } else if (!PCON0bits.nRMCLR) {
            resetType = HardReset;
            PCON0bits.nRMCLR = SET; // Reset reset on MCLR status
        } else if (!PCON0bits.nRI) {
            resetType = SoftResest;
            PCON0bits.nRI = SET; // Reset reset on instruction status
        } else if (!PCON0bits.nRWDT) {
            resetType = WDTReset;
            PCON0bits.nRWDT = SET; // Reset WDT status
        } else if (PCON0bits.STKOVF || PCON0bits.STKUNF) {
            resetType = StackReset;
            PCON0bits.STKOVF = CLEAR; // Reset Stack Overflow status
            PCON0bits.STKUNF = CLEAR; // Reset Stack underflow status
        }
        return resetType;
    }
    return resetType;
}
/*************checkResetType#End**********/

/*************hardResetMenu#Start**********/

/*************************************************************************************************************************

This function is called to go in diagnostic modes on hard system reset
The purpose of this function is to perform selected diagnostic actions on System hard Reset.

 ***************************************************************************************************************************/

void hardResetMenu() {
#ifdef LCD_DISPLAY_ON_H
    lcdClear();
    lcdWriteStringAtCenter("System Running In", 2);
    lcdWriteStringAtCenter("Diagnostic Mode", 3);
    __delay_ms(2000);
#endif
    if (resetCount == 0) {
        for (iterator = 1; iterator < 10; iterator++) {
            resetCount++;
            saveResetCountIntoEeprom();
            switch (resetCount) {
                case 1:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Press Reset Button", 1);
                    lcdWriteStringAtCenter("To Calibrate Motor", 2);
                    lcdWriteStringAtCenter("Current In No Load", 3);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
                    break;
                case 2:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Press Reset Button", 1);
                    lcdWriteStringAtCenter("To Calibrate Motor", 2);
                    lcdWriteStringAtCenter("Current In Full Load", 3);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
                    break;
                case 3:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Press Reset Button", 1);
                    lcdWriteStringAtCenter("To Check GSM Signal", 2);
                    lcdWriteStringAtCenter("Network Strength", 3);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
                    break;
                case 4:
                    break;
                case 5:
                    break;
                case 6:
                    break;
                case 7:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("***Factory Reset****", 1);
                    lcdWriteStringAtCenter("Press Reset Button", 2);
                    lcdWriteStringAtCenter("To Delete System", 3);
                    lcdWriteStringAtCenter("Complete Data", 4);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
                    break;
                case 8:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Press Reset Button", 1);
                    lcdWriteStringAtCenter("To Delete Irrigation", 2);
                    lcdWriteStringAtCenter("Data", 3);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
                    break;
                case 9:
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Press Reset Button", 1);
                    lcdWriteStringAtCenter("To Delete User", 2);
                    lcdWriteStringAtCenter("Registration", 3);
                    __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
            }
        }
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("No Diagnostic Menu", 1);
        lcdWriteStringAtCenter("Selected", 2);
        lcdWriteStringAtCenter("Exiting Diagnostic", 3);
        lcdWriteStringAtCenter("Mode", 4);
        __delay_ms(3000);__delay_ms(3000);__delay_ms(3000);
#endif
        resetCount = 0x00;
        saveResetCountIntoEeprom();
    } else {
        switch (resetCount) {
            case 1:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
#ifdef LCD_DISPLAY_ON_H
                lcdClear();
                lcdWriteStringAtCenter("Calibrating Motor", 2);
                lcdWriteStringAtCenter("In No Load Condition", 3);
                __delay_ms(3000);
#endif
                calibrateMotorCurrent(NoLoad, 0);
                Irri_Motor = OFF; //Manual procedure off
                msgIndex = CLEAR;
                /***************************/
                sendSms(SmsMotor3, userMobileNo, motorLoadRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSystemReset_1_OUT\r\n");
                //********Debug log#end**************//
#endif
                break;
            case 2:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
#ifdef LCD_DISPLAY_ON_H
                lcdClear();
                lcdWriteStringAtCenter("Calibrating Motor", 2);
                lcdWriteStringAtCenter("Full Load Current", 3);
                __delay_ms(3000);
#endif
                calibrateMotorCurrent(FullLoad, 0);
                msgIndex = CLEAR;
                /***************************/
                sendSms(SmsMotor3, userMobileNo, motorLoadRequired);
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSystemReset_2_OUT\r\n");
                //********Debug log#end**************//
#endif
                break;
            case 3:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
#ifdef LCD_DISPLAY_ON_H
                lcdClear();
                lcdWriteStringAtCenter("Checking GSM Signal", 2);
                lcdWriteStringAtCenter("Strength", 3);
                __delay_ms(3000);
#endif
                checkSignalStrength();
                break;
            case 4:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                for (iterator = 0; iterator < 5; iterator++) {
                    //setBCDdigit(0x0F, 1); // BCD Indication for Flash
                    __delay_ms(500);
                    //setBCDdigit(0x04, 1); // BCD Indication for Reset Action#n
                    __delay_ms(1000);
                }
                break;
            case 5:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                for (iterator = 0; iterator < 5; iterator++) {
                    //setBCDdigit(0x0F, 1); // BCD Indication for Flash
                    __delay_ms(500);
                    //setBCDdigit(0x05, 1); // BCD Indication for Reset Action#n
                    __delay_ms(1000);
                }
                break;
            case 6:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                for (iterator = 0; iterator < 5; iterator++) {
                    //setBCDdigit(0x0F, 1); // BCD Indication for Flash
                    __delay_ms(500);
                    //setBCDdigit(0x06, 1); // BCD Indication for Reset Action#n
                    __delay_ms(1000);
                }
                break;
            case 7:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                if (systemAuthenticated) {
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Deleting", 1);
                    lcdWriteStringAtCenter("System Complete Data", 2);
                    lcdWriteStringAtCenter("Resting To", 3);
                    lcdWriteStringAtCenter("Factory Settings", 4);
                    __delay_ms(3000);
#endif
                    deleteValveData();
                    deleteUserData();
                    __delay_ms(1000);
                    loadDataFromEeprom(); // Read configured valve data saved in EEprom
                }
                break;
            case 8:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                if (systemAuthenticated) {
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Deleting", 1);
                    lcdWriteStringAtCenter("Irrigation Data", 2);
                    __delay_ms(3000);
#endif
                    deleteValveData();
                    __delay_ms(1000);
                    loadDataFromEeprom(); // Read configured valve data saved in EEprom
                }
                break;
            case 9:
                resetCount = 0x00;
                saveResetCountIntoEeprom();
                if (systemAuthenticated) {
#ifdef LCD_DISPLAY_ON_H
                    lcdClear();
                    lcdWriteStringAtCenter("Deleting", 1);
                    lcdWriteStringAtCenter("User Registration", 2);
                    lcdWriteStringAtCenter("Data", 3);
                    __delay_ms(3000);
#endif
                    deleteUserData();
                    __delay_ms(1000);
                    loadDataFromEeprom(); // Read configured valve data saved in EEprom
                }
        }
        resetCount = 0x00;
        saveResetCountIntoEeprom();
    }
}

/*************hardResetMenu#End**********/

/*************actionsOnSystemReset#Start**********/

/*************************************************************************************************************************

This function is called to do actions on system reset
The purpose of this function is to perform actions on Power on reset or System hard Reset.

 ***************************************************************************************************************************/
void actionsOnSystemReset(void) {
    unsigned char resetType = CLEAR;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("actionsOnSystemReset_IN\r\n");
    //********Debug log#end**************//
#endif
    configureController(); // set Microcontroller ports, ADC, Timer, I2C, UART, Interrupt Config
    setFactoryPincode();
    resetType = checkResetType();
    if (resetType != HardReset) {
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("System is Booting Up", 2);
        for (int i = 0; i < 10; i++) {
            sprintf(temporaryBytesArray, "%d%c", (i+1)*10,0x25);
            lcdWriteStringAtCenter(temporaryBytesArray, 3);
            __delay_ms(2000);
        } // Warmup 30 seconds
#endif
    }
    else if (resetType == HardReset) {
        readResetCountFromEeprom();
        hardResetMenu(); 
    }
    loadDataFromEeprom(); // Read configured valve data saved in EEprom   
    configureGSM(); // Configure GSM in TEXT mode
    __delay_ms(1000);
    setGsmToLocalTime(); 
    deleteMsgFromSIMStorage(); // Clear GSM storage memory for new Messages
    while (!systemAuthenticated) { // check if System not yet configured
#ifdef LCD_DISPLAY_ON_H
        lcdClear();
        lcdWriteStringAtCenter("Waiting For User", 2);
        lcdWriteStringAtCenter("Registration", 3);
        __delay_ms(3000);
#endif
        //setBCDdigit(0x01, 0); // (1.) BCD Indication for Authentication Error 
        strncpy(pwd, factryPswrd, 6); // consider factory password
        sleepCount = 65500; // Set Default Sleep count until next sleep count is calculated
        deepSleep(); // Sleep with default sleep count until system is configured
        // check if Sleep count executed with interrupt occurred due to new SMS command reception
        if (newSMSRcvd) {
            //setBCDdigit(0x02, 1); // (2) BCD Indication for New SMS Received
            //__delay_ms(500);
            newSMSRcvd = false; // received cmd is processed										
            extractReceivedSms(); // Read received SMS
            deleteMsgFromSIMStorage();
        }
    }
    if (systemAuthenticated) { // check if system is authenticated and valve action is due
        if (phaseFailure()) { // phase failure detected
            sleepCount = 65500;
            /***************************/
            sendSms(SmsSR01, userMobileNo, noInfo); // Acknowledge user about System restarted with phase failure
#ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission 
            //setBCDdigit(0x05, 0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider 
            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
            phaseFailureActionTaken = true;
        } else {
            startFieldNo = 0;
            // check if System is configured
            for (iterator = 0; iterator < fieldCount; iterator++) {
                // check if any field valve status was true after reset
                if (fieldValve[iterator].status == ON) {
                    startFieldNo = iterator; // start action from interrupted field irrigation valve
                    //getDueDate(fieldValve[iterator].offPeriod); // calculate next due date of valve
                    fetchTimefromRTC();
                    /*** Check if System Restarted on next day of Due date ***/
                    // if year over passes ||  if month  over passes ||  if day over passes 
                    if ((currentYY > fieldValve[iterator].nextDueYY) || (currentMM > fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY) || (currentDD > fieldValve[iterator].nextDueDD && currentMM == fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY) || (currentHour > fieldValve[iterator].motorOnTimeHour && currentDD == fieldValve[iterator].nextDueDD && currentMM == fieldValve[iterator].nextDueMM && currentYY == fieldValve[iterator].nextDueYY)) {
                        valveDue = false; // Clear Valve Due
                        fieldValve[iterator].status = OFF;
                        fieldValve[iterator].cyclesExecuted = fieldValve[iterator].cycles;
                        if (fieldValve[iterator].isFertigationEnabled) {
                            if (fieldValve[iterator].fertigationStage == injectPeriod) {
                                fieldValve[iterator].fertigationStage = OFF;
                                fieldValve[iterator].fertigationValveInterrupted = true;
                                remainingFertigationOnPeriod = readActiveSleepCountFromEeprom();
                                saveRemainingFertigationOnPeriod();
                            } else if (fieldValve[iterator].fertigationStage == flushPeriod || fieldValve[iterator].fertigationStage == wetPeriod) {
                                fieldValve[iterator].fertigationStage = OFF;
                            }
                        }
                        __delay_ms(100);
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("System restarted with Due valve on next day\r\n");
                        //********Debug log#end**************//
#endif
                        break;
                    } else { // if system restarted on same day with due valve
                        valveDue = true; // Set valve ON status
#ifdef DEBUG_MODE_ON_H
                        //********Debug log#start************//
                        transmitStringToDebug("System restarted with Due valve on same day\r\n");
                        //********Debug log#end**************//
#endif
                        break;
                    }
                }
            }
            if (valveDue) {
                dueValveChecked = true;
                /***************************/
                // for field no. 01 to 09
                if (iterator < 9) {
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = iterator + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (iterator > 8 && iterator < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = iterator + 39; // To store field no. of valve in action 
                }
                /***************************/
                switch (resetType) {
                    case PowerOnReset:
                        sendSms(SmsSR02, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                    case LowPowerReset:
                        sendSms(SmsSR03, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                    case HardReset:
                        sendSms(SmsSR04, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                    case SoftResest:
                        sendSms(SmsSR05, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                    case WDTReset:
                        sendSms(SmsSR06, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                    case StackReset:
                        sendSms(SmsSR07, userMobileNo, fieldNoRequired); // Acknowledge user about system restarted with Valve action
                        break;
                }
                resetType = CLEAR;
                /***************************/
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission 
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("System restarted with Due valve\r\n");
                //********Debug log#end**************//
#endif
                sleepCount = readActiveSleepCountFromEeprom();
            } else { // check if no valve action is due
                switch (resetType) {
                    case PowerOnReset:
                        sendSms(SmsSR08, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                    case LowPowerReset:
                        sendSms(SmsSR09, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                    case HardReset:
                        sendSms(SmsSR10, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                    case SoftResest:
                        sendSms(SmsSR11, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                    case WDTReset:
                        sendSms(SmsSR12, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                    case StackReset:
                        sendSms(SmsSR13, userMobileNo, noInfo); // Acknowledge user about system restarted with Valve action
                        break;
                }
                resetType = CLEAR;
                /***************************/
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission 
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider 
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
#ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("System restarted W/O Due Valve\r\n");
                //********Debug log#end**************//
#endif
            }
        }
    }
    if (isRTCBatteryDrained()) {
        /***************************/
        sendSms(SmsRTC1, userMobileNo, noInfo); // Acknowledge user about Please replace RTC battery
#ifdef SMS_DELIVERY_REPORT_ON_H
        sleepCount = 2; // Load sleep count for SMS transmission action
        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
        //setBCDdigit(0x05, 0);
        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
        /***************************/
        if (gsmSetToLocalTime) {
            getDateFromGSM(); // Get today's date from Network
            __delay_ms(1000);
            feedTimeInRTC(); // Feed fetched date from network into RTC
            __delay_ms(1000);
        }
    } else if (lowRTCBatteryDetected) {
        lowRTCBatteryDetected = false;
        __delay_ms(100);
        saveRTCBatteryStatus();
        __delay_ms(100);
        if (gsmSetToLocalTime) {
            getDateFromGSM(); // Get today's date from Network
            __delay_ms(1000);
            feedTimeInRTC(); // Feed fetched date from network into RTC
            __delay_ms(1000);
            /***************************/
            sendSms(SmsRTC3, userMobileNo, noInfo); // Acknowledge user about New RTC battery found, system time is set to local time
#ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            //setBCDdigit(0x05, 0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
            /***************************/
        } else {
            /***************************/
            sendSms(SmsRTC4, userMobileNo, noInfo); // Acknowledge user about New RTC battery found, please set system time manually to local time
#ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            //setBCDdigit(0x05, 0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
            /***************************/
        }
    } else if (gsmSetToLocalTime) {
        getDateFromGSM(); // Get today's date from Network
        __delay_ms(1000);
        feedTimeInRTC(); // Feed fetched date from network into RTC
        __delay_ms(1000);
    }
}
/*************actionsOnSystemReset#End**********/

/*************actionsOnSleepCountFinish#Start**********/

/*************************************************************************************************************************

This function is called to do actions after completing sleep count
The purpose of this function is to perform actions after awaking from deep sleep.

 ***************************************************************************************************************************/
void actionsOnSleepCountFinish(void) {
    unsigned char field_No = CLEAR;
    if (valveDue && sleepCount == 0 && !dryRunDetected && !phaseFailureDetected && !onHold && !lowPhaseCurrentDetected) {
        for (field_No = 0; field_No < fieldCount; field_No++) {
            // upon completing first delay start period sleep , switch ON fertigation valve
            if (fieldValve[field_No].status == ON && fieldValve[field_No].isFertigationEnabled && fieldValve[field_No].fertigationStage == wetPeriod) {
                __delay_ms(1000);
                Fert_Motor = ON; // switch on fertigation valve for given field after start period 
                // Injector code             
                // Initialize all count to zero
                injector1OnPeriodCnt = CLEAR;
                injector2OnPeriodCnt = CLEAR;
                injector3OnPeriodCnt = CLEAR;
                injector4OnPeriodCnt = CLEAR;

                injector1OffPeriodCnt = CLEAR;
                injector2OffPeriodCnt = CLEAR;
                injector3OffPeriodCnt = CLEAR;
                injector4OffPeriodCnt = CLEAR;

                injector1CycleCnt = CLEAR;
                injector2CycleCnt = CLEAR;
                injector3CycleCnt = CLEAR;
                injector4CycleCnt = CLEAR;

                // Initialize Injectors values to configured values          
                injector1OnPeriod = fieldValve[field_No].injector1OnPeriod;
                injector2OnPeriod = fieldValve[field_No].injector2OnPeriod;
                injector3OnPeriod = fieldValve[field_No].injector3OnPeriod;
                injector4OnPeriod = fieldValve[field_No].injector4OnPeriod;

                injector1OffPeriod = fieldValve[field_No].injector1OffPeriod;
                injector2OffPeriod = fieldValve[field_No].injector2OffPeriod;
                injector3OffPeriod = fieldValve[field_No].injector3OffPeriod;
                injector4OffPeriod = fieldValve[field_No].injector4OffPeriod;

                injector1Cycle = fieldValve[field_No].injector1Cycle;
                injector2Cycle = fieldValve[field_No].injector2Cycle;
                injector3Cycle = fieldValve[field_No].injector3Cycle;
                injector4Cycle = fieldValve[field_No].injector4Cycle;

                // Initialize injector cycle
                if (injector1OnPeriod > 0) {
                    Irri_Out9 = ON;
                    injector1OnPeriodCnt++;
                }
                if (injector2OnPeriod > 0) {
                    Irri_Out10 = ON;
                    injector2OnPeriodCnt++;
                }
                if (injector3OnPeriod > 0) {
                    Irri_Out11 = ON;
                    injector3OnPeriodCnt++;
                }
                if (injector4OnPeriod > 0) {
                    Irri_Out12 = ON;
                    injector4OnPeriodCnt++;
                }
                fieldValve[field_No].fertigationStage = injectPeriod;
                if (fieldValve[field_No].fertigationValveInterrupted) {
                    fieldValve[field_No].fertigationValveInterrupted = false;
                    remainingFertigationOnPeriod = readRemainingFertigationOnPeriodFromEeprom();
                    sleepCount = remainingFertigationOnPeriod; // Calculate SleepCounnt after fertigation interrupt due to power off
                } else {
                    sleepCount = fieldValve[field_No].fertigationONperiod; // calculate sleep count for fertigation on period 
                }
                __delay_ms(100);
                saveFertigationValveStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                __delay_ms(100);
                saveActiveSleepCountIntoEeprom(); // Save current valve on time 
                __delay_ms(100);

                /***************************/
                // for field no. 01 to 09
                if (field_No < 9) {
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (field_No > 8 && field_No < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                }
                /***************************/

                /***************************/
                sendSms(SmsFert5, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation started action
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
                /*Send sms*/
                break;
            }// Upon completing fertigation on period sleep, switch off fertigation valve
            else if (fieldValve[field_No].status == ON && fieldValve[field_No].isFertigationEnabled && fieldValve[field_No].fertigationStage == injectPeriod) {
                __delay_ms(1000);
                Fert_Motor = OFF; // switch off fertigation valve for given field after on period                
                //Switch off all Injectors after completing fertigation on Period
                Irri_Out9 = OFF;
                Irri_Out10 = OFF;
                Irri_Out11 = OFF;
                Irri_Out12 = OFF;
                fieldValve[field_No].fertigationStage = flushPeriod;
                fieldValve[field_No].fertigationInstance--;
                if (fieldValve[field_No].fertigationInstance == 0) {
                    fieldValve[field_No].isFertigationEnabled = false;
                }
                __delay_ms(100);
                saveFertigationValveValuesIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                __delay_ms(100);
                sleepCount = fieldValve[field_No].onPeriod - (fieldValve[field_No].fertigationDelay + fieldValve[field_No].fertigationONperiod); // calculate sleep count for on period of Valve 
                __delay_ms(100);
                saveActiveSleepCountIntoEeprom(); // Save current valve on time 
                __delay_ms(100);
                /***************************/
                // for field no. 01 to 09
                if (field_No < 9) {
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (field_No > 8 && field_No < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                }
                /***************************/
                if (fertigationDry) { // Fertigation executed with low fertigation level  detection
                    fertigationDry = false;
                    /***************************/
                    sendSms(SmsFert8, userMobileNo, fieldNoRequired); // Acknowledge user about Fertigation stopped action due to low fertilizer level
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider 
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                    /*Send sms*/
                    break;
                } else if (moistureSensorFailed) { // Fertigation executed with level sensor failure
                    moistureSensorFailed = false;
                    /***************************/
                    sendSms(SmsFert7, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation stopped action
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider 
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                    /*Send sms*/
                    break;
                } else { // Fertigation executed without low level detection and without level sensor failure
                    /***************************/
                    sendSms(SmsFert6, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation stopped action
#ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    //setBCDdigit(0x05, 0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider 
                    //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                    /***************************/
                    /*Send sms*/
                    break;
                }
            }// upon completing entire field valve on period switch off field valve
            else if (fieldValve[field_No].status == ON) {
                fieldValve[field_No].status = OFF;
                if (fieldValve[field_No].cyclesExecuted == fieldValve[field_No].cycles) {
                    fieldValve[field_No].cyclesExecuted = 1; //Cycles execution begin after valve due for first time
                } else {
                    fieldValve[field_No].cyclesExecuted++; //Cycles execution record
                }
                valveDue = false;
                valveExecuted = true; // Valve successfully executed 
                startFieldNo = field_No + 1; // scan for next field no.
                __delay_ms(100);
                saveIrrigationValveNoIntoEeprom(field_No);
                __delay_ms(100);
                saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                __delay_ms(100);
                saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                __delay_ms(100);
                if (fieldValve[field_No].isFertigationEnabled) {
                    fieldValve[field_No].fertigationStage = OFF;
                    __delay_ms(100);
                    saveFertigationValveStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                }
                break;
            }
        }
    } else if (onHold) {
        onHold = false;
        for (field_No = 0; field_No < fieldCount; field_No++) {
            if (fieldValve[field_No].status == ON) {
                if (!fieldValve[field_No].isConfigured) {
                    fieldValve[field_No].status = OFF;
                    if (fieldValve[field_No].cyclesExecuted == fieldValve[field_No].cycles) {
                        fieldValve[field_No].cyclesExecuted = 1; //Cycles execution begin after valve due for first time
                    } else {
                        fieldValve[field_No].cyclesExecuted++; //Cycles execution record
                    }
                    if (fieldValve[field_No].fertigationStage == injectPeriod) {
                        Fert_Motor = OFF; // switch off fertigation valve for given field after on period
                        //Switch off all Injectors after completing fertigation on Period
                        Irri_Out9 = OFF;
                        Irri_Out10 = OFF;
                        Irri_Out11 = OFF;
                        Irri_Out12 = OFF;
                        fieldValve[field_No].fertigationStage = OFF;
                        saveFertigationValveStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                        __delay_ms(100);
                    }
                    valveDue = false;
                    valveExecuted = true; // complete valve for hold
                    startFieldNo = field_No + 1; // scan for next field no.
                    __delay_ms(100);
                    saveIrrigationValveNoIntoEeprom(field_No);
                    __delay_ms(100);
                    saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    break;
                } else if (fieldValve[field_No].fertigationStage == wetPeriod) {
                    sleepCount = readActiveSleepCountFromEeprom();
                    sleepCount = (sleepCount + (fieldValve[field_No].onPeriod - fieldValve[field_No].fertigationDelay)); // Calculate Sleep count after fertigation on hold operation  
                    saveActiveSleepCountIntoEeprom();
                    __delay_ms(100);
                    break;
                } else if (fieldValve[field_No].fertigationStage == injectPeriod) {
                    Fert_Motor = OFF; // switch off fertigation valve for given field after on period
                    //Switch off all Injectors after completing fertigation on Period
                    Irri_Out9 = OFF;
                    Irri_Out10 = OFF;
                    Irri_Out11 = OFF;
                    Irri_Out12 = OFF;
                    fieldValve[field_No].fertigationStage = OFF;
                    saveFertigationValveStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                    __delay_ms(100);
                    sleepCount = readActiveSleepCountFromEeprom();
                    sleepCount = (sleepCount + (fieldValve[field_No].onPeriod - (fieldValve[field_No].fertigationDelay + fieldValve[field_No].fertigationONperiod))); // Calculate Sleep count during fertigation hold operation
                    saveActiveSleepCountIntoEeprom();
                    __delay_ms(100);
                    break;
                }
            }
        }
    }
}
/*************actionsOnSleepCountFinish#End**********/

/*************actionsOnDueValve#Start**********/

/*************************************************************************************************************************

This function is called to do actions on due valve
The purpose of this function is to perform actions after valve is due.

 ***************************************************************************************************************************/
void actionsOnDueValve(unsigned char field_No) {
    unsigned char last_Field_No = CLEAR;
    wetSensor = false;
    // Check if Field is wet
    if (isFieldMoistureSensorWetLora(field_No)) { //Skip current valve execution and go for next
        wetSensor = true;
        valveDue = false;
        fieldValve[field_No].status = OFF;
        fieldValve[field_No].cyclesExecuted = fieldValve[field_No].cycles;
        startFieldNo = field_No + 1; // scan for next field no.
        __delay_ms(50);
        getDueDate(fieldValve[field_No].offPeriod); // calculate next due date of valve
        __delay_ms(50); // Today's date is not known for next due date
        fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
        fieldValve[field_No].nextDueMM = dueMM;
        fieldValve[field_No].nextDueYY = dueYY;
        __delay_ms(100);
        saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
        __delay_ms(100);
        saveIrrigationValveCycleStatusIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
        __delay_ms(100);
        saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
        __delay_ms(100);

        /***************************/
        // for field no. 01 to 09
        if (field_No < 9) {
            temporaryBytesArray[0] = 48; // To store field no. of valve in action 
            temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
        }// for field no. 10 to 12
        else if (field_No > 8 && field_No < 12) {
            temporaryBytesArray[0] = 49; // To store field no. of valve in action 
            temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
        }
        /***************************/

        /***************************/
        sendSms(SmsIrr6, userMobileNo, fieldNoRequired); // Acknowledge user about Irrigation not started due to wet field detection						
#ifdef SMS_DELIVERY_REPORT_ON_H
        sleepCount = 2; // Load sleep count for SMS transmission action
        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
        //setBCDdigit(0x05, 0);
        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
        //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
        /***************************/
    }        
    else if (!phaseFailure()) { // All phase present
        __delay_ms(100);
        activateValve(field_No); // Activate valve for field
        if (!LoraConnectionFailed) { // Skip next block if activation is failed
            __delay_ms(100);

            //Switch ON Fertigation valve interrupted due to power on same day
            if (fieldValve[field_No].fertigationStage == injectPeriod) {
                powerOnMotor(); // Power ON Motor							 
                __delay_ms(1000);
                Fert_Motor = ON;
                // Injector code
                // Initialize all count to zero
                injector1OnPeriodCnt = CLEAR;
                injector2OnPeriodCnt = CLEAR;
                injector3OnPeriodCnt = CLEAR;
                injector4OnPeriodCnt = CLEAR;

                injector1OffPeriodCnt = CLEAR;
                injector2OffPeriodCnt = CLEAR;
                injector3OffPeriodCnt = CLEAR;
                injector4OffPeriodCnt = CLEAR;

                injector1CycleCnt = CLEAR;
                injector2CycleCnt = CLEAR;
                injector3CycleCnt = CLEAR;
                injector4CycleCnt = CLEAR;

                // Initialize Injectors values to configured values
                injector1OnPeriod = fieldValve[field_No].injector1OnPeriod;
                injector2OnPeriod = fieldValve[field_No].injector2OnPeriod;
                injector3OnPeriod = fieldValve[field_No].injector3OnPeriod;
                injector4OnPeriod = fieldValve[field_No].injector4OnPeriod;

                injector1OffPeriod = fieldValve[field_No].injector1OffPeriod;
                injector2OffPeriod = fieldValve[field_No].injector2OffPeriod;
                injector3OffPeriod = fieldValve[field_No].injector3OffPeriod;
                injector4OffPeriod = fieldValve[field_No].injector4OffPeriod;

                injector1Cycle = fieldValve[field_No].injector1Cycle;
                injector2Cycle = fieldValve[field_No].injector2Cycle;
                injector3Cycle = fieldValve[field_No].injector3Cycle;
                injector4Cycle = fieldValve[field_No].injector4Cycle;

                // Initialize injector cycle
                if (injector1OnPeriod > 0) {
                    Irri_Out9 = ON;
                    injector1OnPeriodCnt++;
                }
                if (injector2OnPeriod > 0) {
                    Irri_Out10 = ON;
                    injector2OnPeriodCnt++;
                }
                if (injector3OnPeriod > 0) {
                    Irri_Out11 = ON;
                    injector3OnPeriodCnt++;
                }
                if (injector4OnPeriod > 0) {
                    Irri_Out12 = ON;
                    injector4OnPeriodCnt++;
                }
                /***************************/
                // for field no. 01 to 09
                if (field_No < 9) {
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (field_No > 8 && field_No < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = field_No + 39; // To store field no. of valve in action 
                }
                /***************************/

                /***************************/
                sendSms(SmsFert5, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation started action
#ifdef SMS_DELIVERY_REPORT_ON_H
                sleepCount = 2; // Load sleep count for SMS transmission action
                sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                //setBCDdigit(0x05, 0);
                deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider           
                //setBCDdigit(0x0F, 0); // Blank "." BCD Indication for Normal Condition
#endif
                /***************************/
                /*Send sms*/

            } else if (valveExecuted) { // DeActivate previous executed field valve
                last_Field_No = readFieldIrrigationValveNoFromEeprom();
                if (last_Field_No != field_No) { // if not multiple cycles for same valve
                    deActivateValve(last_Field_No); // Successful Deactivate valve 
                }
                valveExecuted = false;
            } else { // Switch on Motor for First Valve activation
                powerOnMotor(); // Power ON Motor
            }

            if (fieldValve[field_No].cyclesExecuted == fieldValve[field_No].cycles) {
                /******** Calculate and save Field Valve next Due date**********/
                getDueDate(fieldValve[field_No].offPeriod); // calculate next due date of valve
                fieldValve[field_No].nextDueDD = (unsigned char) dueDD;
                fieldValve[field_No].nextDueMM = dueMM;
                fieldValve[field_No].nextDueYY = dueYY;
                __delay_ms(100);
                saveIrrigationValveDueTimeIntoEeprom(eepromAddress[field_No], &fieldValve[field_No]);
                __delay_ms(100);
                /***********************************************/
            }
        }
    }
}
/*************actionsOnDueValve#End**********/


/*************deleteUserDataOnRequest#Start**********/

/*************************************************************************************************************************

This function is called to delete user data
The purpose of this function is to delete user data and informed user about deletion

 ***************************************************************************************************************************/
void deleteUserData(void) {
    sendSms(SmsSR14, userMobileNo, noInfo);
    systemAuthenticated = false;
    saveAuthenticationStatus();
    for (iterator = 0; iterator < 10; iterator++) {
        userMobileNo[iterator] = '0';
    }
    saveMobileNoIntoEeprom();
}
/*************deleteUserDataOnRequest#End**********/


/*************deleteValveDataOnRequest#Start**********/

/*************************************************************************************************************************

This function is called to delete valve configuration data
The purpose of this function is to delete user configured valve data and informed user about deletion

 ***************************************************************************************************************************/
void deleteValveData(void) {
    sendSms(SmsSR14, userMobileNo, noInfo);
    filtrationDelay1 = 0;
    filtrationDelay2 = 0;
    filtrationDelay3 = 0;
    filtrationOnTime = 0;
    filtrationSeperationTime = 0;
    filtrationEnabled = false;
    __delay_ms(100);
    saveFiltrationSequenceData();
    __delay_ms(100);
    for (iterator = 0; iterator < fieldCount; iterator++) {
        msgIndex = CLEAR;
        fieldValve[iterator].status = OFF;
        fieldValve[iterator].isConfigured = false;
        fieldValve[iterator].fertigationDelay = 0;
        fieldValve[iterator].fertigationONperiod = 0;
        fieldValve[iterator].fertigationInstance = 0;
        fieldValve[iterator].fertigationStage = OFF;
        fieldValve[iterator].fertigationValveInterrupted = false;
        fieldValve[iterator].isFertigationEnabled = false;

        saveIrrigationValveOnOffStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
        __delay_ms(100);
        saveIrrigationValveConfigurationStatusIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
        __delay_ms(100);
        saveFertigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
        __delay_ms(100);
    }
}
/*************deleteValveDataOnRequest#End**********/

/*************randomPasswordGeneration#Start**********/

/*************************************************************************************************************************

This function is called to generate 6 digit password
The purpose of this function is to randomly generate password of length 6

 ***************************************************************************************************************************/
void randomPasswordGeneration(void) {
    // Seed the random-number generator
    // with current time so that the
    // numbers will be different every time
    getDateFromGSM();
    srand((unsigned int) (currentDD + currentHour + currentMinutes + currentSeconds));

    // Array of numbers
    unsigned char numbers[] = "0123456789";

    // Iterate over the range [0, N]
    for (iterator = 0; iterator < 6; iterator++) {
        factryPswrd[iterator] = numbers[rand() % 10];
    }
    factryPswrd[6] = '\0';
}

/*************delete gsmResponse string#Start**********/

/*************************************************************************************************************************

This function is called to delete gsm response string
The purpose of this function is to enter null values in gsm response

 ***************************************************************************************************************************/
void deleteGsmResponse(void) {
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteGsmResponse_IN\r\n");
    //********Debug log#end**************//
#endif
    // Iterate over the range [0, N]
    for (iterator = 0; iterator < 200; iterator++) {
        gsmResponse[iterator] = '\0';
    }
    msgIndex = CLEAR;
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteGsmResponse_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************delete StringToDecode string#Start**********/

/*************************************************************************************************************************

This function is called to delete stringToDecode  string
The purpose of this function is to enter null values in stringToDecode

 ***************************************************************************************************************************/
void deleteStringToDecode(void) {
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteStringToDecode_IN\r\n");
    //********Debug log#end**************//
#endif
    // Iterate over the range [0, N]
    for (iterator = 0; iterator < 200; iterator++) {
        stringToDecode[iterator] = '\0';
    }
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteStringToDecode_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************delete Decoded string#Start**********/

/*************************************************************************************************************************

This function is called to delete Decoded string
The purpose of this function is to enter null values in Decoded string

 ***************************************************************************************************************************/
void deleteDecodedString(void) {
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteDecodedString_IN\r\n");
    //********Debug log#end**************//
#endif
    // Iterate over the range [0, N]
    for (iterator = 0; iterator < 200; iterator++) {
        decodedString[iterator] = '\0';
    }
    /***************************/
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteDecodedString_OUT\r\n");
    //********Debug log#end**************//
#endif
}