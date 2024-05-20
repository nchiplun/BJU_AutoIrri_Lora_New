/*
 * File name            : eeprom.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : eeprom functions declaration source file
 */

#include "congfigBits.h"
#include "variableDefinitions.h"
#include "controllerActions.h"
#include "i2c_LCD_PCF8574.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif

//**********************EEPROM Data organization functions_start***************//

/*************************************************************************************************************************

This function is called to write Data into EEPROM memory
The purpose of this function is to load 8 bit data into given address. 

 **************************************************************************************************************************/
void eepromWrite(unsigned int address, unsigned char data) {
    NVMADRL = address & 0xFF;
    NVMADRH = (address >> 8) & 0xFF; //address location
    NVMDAT = data; //data
    NVMCON1bits.NVMREG = 0b00; //Setup Data EEPROM access
    NVMCON1bits.WREN = ENABLED; //enable write
    INTCONbits.GIE = DISABLED; //disable global interrupt
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = SET;
    while (NVMCON1bits.WR == SET);
    // ADD indication if infinite
    INTCONbits.GIE = ENABLED;
    NVMCON1bits.WREN = DISABLED; //disable write operation
}

/*************************************************************************************************************************

This function is called to read Data fromEEPROM memory
The purpose of this function is to load 8 bit data from given address. 

 **************************************************************************************************************************/
unsigned char eepromRead(unsigned int address) {
    NVMADRL = address & 0xFF;
    NVMADRH = (address >> 8) & 0xFF; //address location
    NVMCON1bits.NVMREG = 0b00; //Setup Data EEPROM access
    NVMCON1bits.RD = ENABLED; //enable read
    __delay_ms(5);
    return NVMDAT;
}
//**********************EEPROM Data organization functions_end***************//

//************************read structures from eeprom_start*************************//

/*************************************************************************************************************************

This function is called to save field valve configuration into EEPROM memory
The purpose of this function is to load entire Field valve structure into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveIrrigationValveValuesIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveValuesIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address, fieldptr->dryValue & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 1, (fieldptr->dryValue >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 2, fieldptr->wetValue & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 3, (fieldptr->wetValue >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 4, fieldptr->onPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 5, (fieldptr->onPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 30, fieldptr->offPeriod);
    __delay_ms(50);
    eepromWrite(address + 31, fieldptr->motorOnTimeHour);
    __delay_ms(50);
    eepromWrite(address + 32, fieldptr->motorOnTimeMinute);
    __delay_ms(50);
    eepromWrite(address + 37, fieldptr->priority);
    __delay_ms(50);
    eepromWrite(address + 39, fieldptr->cycles);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveValuesIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/*************************************************************************************************************************

This function is called to save field fertigation valve configuration into EEPROM memory
The purpose of this function is to load entire Field fertigation valve structure into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveFertigationValveValuesIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFertigationValveValuesIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 6, fieldptr->fertigationDelay & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 7, (fieldptr->fertigationDelay >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 8, fieldptr->fertigationONperiod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 9, (fieldptr->fertigationONperiod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 10, fieldptr->injector1OnPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 11, (fieldptr->injector1OnPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 12, fieldptr->injector2OnPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 13, (fieldptr->injector2OnPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 14, fieldptr->injector3OnPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 15, (fieldptr->injector3OnPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 16, fieldptr->injector4OnPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 17, (fieldptr->injector4OnPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 18, fieldptr->injector1OffPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 19, (fieldptr->injector1OffPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 20, fieldptr->injector2OffPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 21, (fieldptr->injector2OffPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 22, fieldptr->injector3OffPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 23, (fieldptr->injector3OffPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 24, fieldptr->injector4OffPeriod & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 25, (fieldptr->injector4OffPeriod >> 8) & 0xFF);
    __delay_ms(50);
    eepromWrite(address + 26, fieldptr->injector1Cycle);
    __delay_ms(50);
    eepromWrite(address + 27, fieldptr->injector2Cycle);
    __delay_ms(50);
    eepromWrite(address + 28, fieldptr->injector3Cycle);
    __delay_ms(50);
    eepromWrite(address + 29, fieldptr->injector4Cycle);
    __delay_ms(50);
    eepromWrite(address + 36, fieldptr->fertigationInstance);
    __delay_ms(50);
    eepromWrite(address + 38, fieldptr->fertigationStage);
    __delay_ms(50);
    eepromWrite(address + 43, fieldptr->isFertigationEnabled);
    __delay_ms(50);
    eepromWrite(address + 44, fieldptr->fertigationValveInterrupted);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFertigationValveValuesIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save field valve Due time into EEPROM memory
The purpose of this function is to load due date time stamp of Field valve into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom
void saveIrrigationValveDueTimeIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveDueTimeIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 33, fieldptr->nextDueDD);
    __delay_ms(50);
    eepromWrite(address + 34, fieldptr->nextDueMM);
    __delay_ms(50);
    eepromWrite(address + 35, fieldptr->nextDueYY);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveDueTimeIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save field valve on off status into EEPROM memory
The purpose of this function is to load ON OFF status of Field valve into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveIrrigationValveOnOffStatusIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveOnOffStatusIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 41, fieldptr->status);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveOnOffStatusIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save field valve cycle execution status into EEPROM memory
The purpose of this function is to load no. of cycles executed status of Field valve into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveIrrigationValveCycleStatusIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveCycleStatusIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 40, fieldptr->cyclesExecuted);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveCycleStatusIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save fertigation valve on off status into EEPROM memory
The purpose of this function is to load ON OFF status of Field valve into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveFertigationValveStatusIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFertigationValveStatusIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 36, fieldptr->fertigationInstance);
    __delay_ms(50);
    eepromWrite(address + 38, fieldptr->fertigationStage);
    __delay_ms(50);
    eepromWrite(address + 44, fieldptr->fertigationValveInterrupted);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFertigationValveStatusIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save field valve HOLD/Resume status into EEPROM memory
The purpose of this function is to load Hold/ Resume configuration status of Field valve into the assigned EEPROM memory

 **************************************************************************************************************************/
//Write field structure into eeprom 
void saveIrrigationValveConfigurationStatusIntoEeprom(unsigned int address, struct FIELDVALVE *fieldptr) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveConfigurationStatusIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(address + 42, fieldptr->isConfigured);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveConfigurationStatusIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to read field valve configuration from EEPROM memory
The purpose of this function is to load entire Field valve structure from the assigned EEPROM memory

 **************************************************************************************************************************/
//Read field structure from eeprom 
void readValveDataFromEeprom(unsigned int address, struct FIELDVALVE *fieldptr){
    ////setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    __delay_ms(50);
    lower8bits = eepromRead(address);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 1);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->dryValue = ((lower8bits) | (higher8bits));
    lower8bits = eepromRead(address + 2);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 3);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->wetValue = ((lower8bits) | (higher8bits));
    lower8bits = eepromRead(address + 4);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 5);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->onPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 6);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 7);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->fertigationDelay = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 8);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 9);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->fertigationONperiod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 10);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 11);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector1OnPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 12);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 13);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector2OnPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 14);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 15);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector3OnPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 16);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 17);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector4OnPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 18);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 19);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector1OffPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 20);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 21);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector2OffPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 22);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 23);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector3OffPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    lower8bits = eepromRead(address + 24);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(address + 25);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fieldptr->injector4OffPeriod = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    fieldptr->injector1Cycle = eepromRead(address + 26);
    __delay_ms(50);
    fieldptr->injector2Cycle = eepromRead(address + 27);
    __delay_ms(50);
    fieldptr->injector3Cycle = eepromRead(address + 28);
    __delay_ms(50);
    fieldptr->injector4Cycle = eepromRead(address + 29);
    __delay_ms(50);    
    fieldptr->offPeriod = eepromRead(address + 30);
    __delay_ms(50);
    fieldptr->motorOnTimeHour = eepromRead(address + 31);
    __delay_ms(50);
    fieldptr->motorOnTimeMinute = eepromRead(address + 32);
    __delay_ms(50);
    fieldptr->nextDueDD = eepromRead(address + 33);
    __delay_ms(50);
    fieldptr->nextDueMM = eepromRead(address + 34);
    __delay_ms(50);
    fieldptr->nextDueYY = eepromRead(address + 35);
    __delay_ms(50);
    fieldptr->fertigationInstance = eepromRead(address + 36);
    __delay_ms(50);
    fieldptr->priority = eepromRead(address + 37);
    __delay_ms(50);
    fieldptr->fertigationStage = eepromRead(address + 38);
    __delay_ms(50);
    fieldptr->cycles = eepromRead(address + 39);
    __delay_ms(50);
    fieldptr->cyclesExecuted = eepromRead(address + 40);
    __delay_ms(50);
    fieldptr->status = eepromRead(address + 41);
    __delay_ms(50);
    fieldptr->isConfigured = eepromRead(address + 42);
    __delay_ms(50);
    fieldptr->isFertigationEnabled = eepromRead(address + 43);
    __delay_ms(50);
    fieldptr->fertigationValveInterrupted = eepromRead(address + 44);
    __delay_ms(50);    
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
}

/*************************************************************************************************************************

This function is called to save Registered user mobile no. into EEPROM memory
The purpose of this function is to load Registered mobile no. into assigned memory.

 **************************************************************************************************************************/
void saveMobileNoIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveMobileNoIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    for (iterator = 0; iterator < 10; iterator++) {
        __delay_ms(50);
        eepromWrite(eepromAddress[forMobileNo] + iterator, userMobileNo[iterator]);
    }
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveMobileNoIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save Registered user Password into EEPROM memory
The purpose of this function is to load Registered Password into assigned memory.

 **************************************************************************************************************************/
void savePasswordIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("savePasswordIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    for (iterator = 0; iterator < 6; iterator++) {
        __delay_ms(50);
        eepromWrite(eepromAddress[forPassword] + iterator, pwd[iterator]);
    }
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("savePasswordIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save Field - Valve Mapping into EEPROM memory
The purpose of this function is to load Field mapping into assigned memory.

 **************************************************************************************************************************/
void saveFieldMappingIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("savePasswordIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    for (iterator = 13; iterator < 37; iterator++) {
        __delay_ms(50);
        eepromWrite(eepromAddress[forSystem] + iterator, fieldMap[iterator - 13]);
    }
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("savePasswordIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save Factory Set Password into EEPROM memory
The purpose of this function is to load Factory Set Password into assigned memory.

 **************************************************************************************************************************/
void saveFactryPswrdIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFactryPswrdIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    for (iterator = 7; iterator < 13; iterator++) {
        __delay_ms(50);
        eepromWrite(eepromAddress[forPassword] + iterator, factryPswrd[iterator-7]);
    }
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFactryPswrdIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save sleep count into EEPROM memory
The purpose of this function is to load sleep count of active valve on period into assigned memory.

 **************************************************************************************************************************/
void saveActiveSleepCountIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveActiveSleepCountIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 1, sleepCount & 0xFF); // LOWER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 2, (sleepCount >> 8) & 0xFF); //HIGHER 8 BIT
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveActiveSleepCountIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save reset count into EEPROM memory
The purpose of this function is to store reset count occurred by MCLR reset to show menu option on reset.

 **************************************************************************************************************************/
void saveResetCountIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveResetCountIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 7, resetCount); 
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveResetCountIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save Device Burn info into EEPROM memory
The purpose of this function is to store Device programming status.

 **************************************************************************************************************************/
void saveDeviceProgramStatusIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveDeviceProgramStatusIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 8, DeviceBurnStatus); 
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveDeviceProgramStatusIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save motor load configuration into EEPROM memory
The purpose of this function is to load motor no load and full load values into the assigned EEPROM memory

**************************************************************************************************************************/ 
void saveMotorLoadValuesIntoEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveMotorLoadValuesIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 9, noLoadCutOff & 0xFF); // LOWER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 10, (noLoadCutOff >> 8) & 0xFF); //HIGHER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 11, fullLoadCutOff & 0xFF); // LOWER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 12, (fullLoadCutOff >> 8) & 0xFF); //HIGHER 8 BIT
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveMotorLoadValuesIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to read motor load configuration from EEPROM memory
The purpose of this function is to load motor no load and full load values from the assigned EEPROM memory

 **************************************************************************************************************************/
void readMotorLoadValuesFromEeprom(void){
    ////setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    __delay_ms(50);
    lower8bits = eepromRead(eepromAddress[forSystem] + 9);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(eepromAddress[forSystem] + 10);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    noLoadCutOff = ((lower8bits) | (higher8bits));
    lower8bits = eepromRead(eepromAddress[forSystem] + 11);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(eepromAddress[forSystem] + 12);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    fullLoadCutOff = ((lower8bits) | (higher8bits));
    __delay_ms(50);  
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
}

/*************************************************************************************************************************

This function is called to read saved reset count into EEPROM memory
The purpose of this function is to extract stored reset count occurred by MCLR reset to show menu option on reset.

 **************************************************************************************************************************/
void readResetCountFromEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readResetCountFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
//    //setBCDdigit(0x04,1);
//    __delay_ms(50);
    resetCount = eepromRead(eepromAddress[forSystem] + 7); 
    __delay_ms(50);
//    //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readResetCountFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to read Device Burn info from EEPROM memory
The purpose of this function is to fetch Device programming status.

 **************************************************************************************************************************/
void readDeviceProgramStatusFromEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readDeviceProgramStatusFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
//    //setBCDdigit(0x04,1);
//    __delay_ms(50);
    DeviceBurnStatus = eepromRead(eepromAddress[forSystem] + 8); 
    __delay_ms(50);
//    //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readDeviceProgramStatusFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save fertigation remaining sleep count into EEPROM memory
The purpose of this function is to load fertigation interrupted sleep count into assigned memory.

 **************************************************************************************************************************/
void saveRemainingFertigationOnPeriod(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveRemainingFertigationOnPeriod_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 3, remainingFertigationOnPeriod & 0xFF); // LOWER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 4, (remainingFertigationOnPeriod >> 8) & 0xFF); //HIGHER 8 BIT
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveRemainingFertigationOnPeriod_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to read sleep count from EEPROM memory
The purpose of this function is to load 16 bit sleep count of Active valve on period from assigned memory.

 **************************************************************************************************************************/
unsigned int readActiveSleepCountFromEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readActiveSleepCountFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    __delay_ms(50);
    lower8bits = eepromRead(eepromAddress[forSystem] + 1);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(eepromAddress[forSystem] + 2);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readActiveSleepCountFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
    return ((lower8bits) | (higher8bits));
}


/*************************************************************************************************************************

This function is called to read sleep count from EEPROM memory
The purpose of this function is to load 16 bit sleep count from assigned memory.

 **************************************************************************************************************************/
unsigned int readRemainingFertigationOnPeriodFromEeprom(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readRemainingFertigationOnPeriodFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    __delay_ms(50);
    lower8bits = eepromRead(eepromAddress[forSystem] + 3);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(eepromAddress[forSystem] + 4);
    __delay_ms(50);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readRemainingFertigationOnPeriodFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
    return ((lower8bits) | (higher8bits));
}

/*************************************************************************************************************************

This function is called to save authentication status into EEPROM memory
The purpose of this function is to load System Authentication status into assigned memory

 **************************************************************************************************************************/
void saveAuthenticationStatus(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveAuthenticationStatus_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem], systemAuthenticated);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveAuthenticationStatus_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to RTC Battery Status into EEPROM memory
The purpose of this function is to store RTC Battery Status on measuring its voltage level into assigned memory

 **************************************************************************************************************************/
void saveRTCBatteryStatus(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveRTCBatteryStatus_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 6, lowRTCBatteryDetected);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveRTCBatteryStatus_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save field no. into EEPROM memory
The purpose of this function is to save field no. into assigned memory

 **************************************************************************************************************************/
void saveIrrigationValveNoIntoEeprom(unsigned char field_no) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveNoIntoEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forSystem] + 5, field_no);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveIrrigationValveNoIntoEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save filtration sequence dataR into EEPROM memory
The purpose of this function is to store filtration delay values on configuring into system

 **************************************************************************************************************************/
void saveFiltrationSequenceData(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFiltrationSequenceData_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,0); //  "4." BCD Indication for EEPROM Memory Write Operation
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration], filtrationDelay1);
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 1, filtrationDelay2);
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 2, filtrationDelay3);
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 3, filtrationOnTime);
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 4, filtrationSeperationTime & 0xFF); // LOWER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 5, (filtrationSeperationTime >> 8) & 0xFF); //HIGHER 8 BIT
    __delay_ms(50);
    eepromWrite(eepromAddress[forFiltration] + 6, filtrationEnabled);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("saveFiltrationSequenceData_OUT\r\n");
    //********Debug log#end**************//
#endif
}
/*************************************************************************************************************************

This function is called to read field no. from EEPROM memory
The purpose of this function is to read field no. from assigned memory

 **************************************************************************************************************************/
unsigned char readFieldIrrigationValveNoFromEeprom(void) {
    unsigned char field_no = 0;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readFieldIrrigationValveNoFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    ////setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    __delay_ms(50);
    field_no = eepromRead(eepromAddress[forSystem] + 5);
    __delay_ms(50);
    ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("readFieldIrrigationValveNoFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
    return field_no;
}

/*************************************************************************************************************************

This function is called to read all system data from EEPROM memory
The purpose of this function is to load all system data from assigned EEPROM memory after system boot/ power up.

 **************************************************************************************************************************/
//read data like all structures and bit values from eeprom

void loadDataFromEeprom(void) {
    lcdClear();
    lcdWriteStringAtCenter("Loading System Data", 2);
    lcdWriteStringAtCenter("1%", 3);
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("loadDataFromEeprom_IN\r\n");
    //********Debug log#end**************//
#endif
    //setBCDdigit(0x04,1); // "4" BCD Indication for EEPROM Memory Read Operation
    for (iterator = 0; iterator < fieldCount; iterator++) {
        __delay_ms(50);
        readValveDataFromEeprom(eepromAddress[iterator], &fieldValve[iterator]);
        sprintf(temporaryBytesArray,"%d%c",(iterator+1)*6,0x25);
        lcdWriteStringAtCenter(temporaryBytesArray, 3);
    }
    for (iterator = 0; iterator < 10; iterator++) {
        __delay_ms(50);
        userMobileNo[iterator] = eepromRead(eepromAddress[forMobileNo] + iterator);
    }
    lcdWriteStringAtCenter("96%", 3);
    userMobileNo[10] = '\0';
    for (iterator = 0; iterator < 6; iterator++) {
        __delay_ms(50);
        pwd[iterator] = eepromRead(eepromAddress[forPassword] + iterator);
    }
    lcdWriteStringAtCenter("97%", 3);
    pwd[6] = '\0';
    for (iterator = 7; iterator < 13; iterator++) {
        __delay_ms(50);
        factryPswrd[iterator-7] = eepromRead(eepromAddress[forPassword] + iterator);
    }
    lcdWriteStringAtCenter("98%", 3);
    factryPswrd[6] = '\0';
     for (iterator = 13; iterator < 37; iterator++) {
        __delay_ms(50);
        fieldMap[iterator - 13] = eepromRead(eepromAddress[forSystem] + iterator);
    }
    lcdWriteStringAtCenter("99%", 3);
    __delay_ms(50);
    systemAuthenticated = eepromRead(eepromAddress[forSystem]);
    __delay_ms(50);
    lowRTCBatteryDetected = eepromRead(eepromAddress[forSystem] + 6);
    __delay_ms(50);
    //resetCount = eepromRead(eepromAddress[forSystem] + 7); 
    //__delay_ms(50);
    //DeviceBurnStatus = eepromRead(eepromAddress[forSystem] + 8); 
    //__delay_ms(50);
    filtrationDelay1 = eepromRead(eepromAddress[forFiltration]);
    __delay_ms(50);
    filtrationDelay2 = eepromRead(eepromAddress[forFiltration]+1);
    __delay_ms(50);
    filtrationDelay3 = eepromRead(eepromAddress[forFiltration]+2);
    __delay_ms(50);
    filtrationOnTime = eepromRead(eepromAddress[forFiltration]+3);
    __delay_ms(50);
    lower8bits = eepromRead(eepromAddress[forFiltration] + 4);
    __delay_ms(50);
    lower8bits &= 0x00FF;
    higher8bits = eepromRead(eepromAddress[forFiltration] + 5);
    higher8bits <<= 8;
    higher8bits &= 0xFF00;
    filtrationSeperationTime = ((lower8bits) | (higher8bits));
    __delay_ms(50);
    filtrationEnabled = eepromRead(eepromAddress[forFiltration]+6);
    __delay_ms(50);
    readMotorLoadValuesFromEeprom();
    lcdWriteStringAtCenter("100%", 3);
    __delay_ms(1000);
    //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("loadDataFromEeprom_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to save all the system data into EEPROM memory
The purpose of this function is to load all system data into assigned EEPROM memory before system shutdown/power down.

**************************************************************************************************************************/
//write data like all structures and bit values into eeprom
/*
void loadDataIntoEeprom(void) {
    for (iterator = 0; iterator < fieldCount; iterator++) {
        saveIrrigationValveValuesIntoEeprom(eepromAddress[iterator], &fieldValve[iterator]);
        __delay_ms(50);
    }
    saveMobileNoIntoEeprom();
    __delay_ms(50);
    savePasswordIntoEeprom();
    __delay_ms(50);
    saveAuthenticationStatus();
    __delay_ms(50);
}
*/
/************************read structures from eeprom_end***********************/