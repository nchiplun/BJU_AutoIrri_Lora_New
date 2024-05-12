/*
 * File name            : Main.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : Main source file
 */

/***************************** Header file declarations#start ************************/

#include "congfigBits.h"
#include "variableDefinitions.h"
#include "controllerActions.h"
#include "eeprom.h"
#include "gsm.h"
#include "lora.h"
#include "i2c.h"
#include "i2c_LCD_PCF8574.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif
/***************************** Header file declarations#end **************************/

//**************interrupt service routine handler#start***********//

/*************************************************************************************************************************

This function is called when an interrupt has occurred at RX pin of ?c which is connected to TX pin of GSM.
Interrupt occurs at 1st cycle of each Data byte.
The purpose of this interrupt handler is to store the data received from GSM into Array called gsmResponse[]
Each response from GSM starts with '+' symbol, e.g. +CMTI: "SM", <index>
The End of SMS is detected by OK command.

 **************************************************************************************************************************/

void __interrupt(high_priority)rxANDiocInterrupt_handler(void) {
    if (PIR4bits.RC3IF) { // Interrupt on GSM RX pin
        Run_led = DARK; // Led Indication for system in Operational Mode
        rxCharacter = rxByte(); // Read byte received at Reception Register
        // Check if any overrun occur due to continuous reception
        if (RC3STAbits.OERR) {
            RC3STAbits.CREN = 0;
            Nop();
            RC3STAbits.CREN = 1;
        }
        // If interrupt occurred in sleep mode due to command from GSM
        if (inSleepMode) {
            //SIM_led = GLOW;  // Led Indication for GSM interrupt in sleep mode 
            //sleepCount = 2; // Set minimum sleep to 2 for recursive reading from GSM
            //sleepCountChangedDueToInterrupt = true; // Sleep count is altered and hence needs to be read from memory
            // check if GSM initiated communication with '+'
            if (rxCharacter == '+') {
                msgIndex = CLEAR; // Reset message storage index to first character to start reading from '+'
                gsmResponse[msgIndex] = rxCharacter; // Load Received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
            } else if (msgIndex < 12 && cmti[msgIndex] == rxCharacter) { // Check if Sms type cmd is initiated and received byte is cmti command
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
                if (msgIndex == 12) { // check if storage index is reached to last character of CMTI command
                    cmtiCmd= true; // Set to indicate cmti command received	
                }
            } else if (cmtiCmd && msgIndex == 12) { //To extract sim location for SMS storage
                cmtiCmd= false; // reset for next cmti command reception	
                temporaryBytesArray[0] = rxCharacter; // To store sim memory location of received message
                msgIndex = CLEAR;
                newSMSRcvd = true; // Set to indicate New SMS is Received
                //sleepCount = 1;
                //sleepCountChangedDueToInterrupt = true; // Sleep count is altered and hence needs to be calculated again
            }
        } else if (!controllerCommandExecuted) { // check if GSM response to µc command is not completed
            //SIM_led = GLOW;  // Led Indication for GSM interrupt in operational mode
            // Start storing response if received data is '+' at index zero
            if (rxCharacter == '+' && msgIndex == 0) {
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
            } else if (msgIndex > 0 && msgIndex <=200) { // Cascade received data to stored response after receiving first character '+'
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                if (gsmResponse[msgIndex - 1] == 'O' && gsmResponse[msgIndex] == 'K') { // Cascade till 'OK'  is found
                    controllerCommandExecuted = true; // GSM response to µc command is completed
                    msgIndex = CLEAR; // Reset message storage index to first character to start reading for next received byte of cmd
                } else if (msgIndex <= 200) { // Read bytes till 200 characters
                    msgIndex++;
                }
            }
        }
        //SIM_led = DARK;  // Led Indication for GSM interrupt is done 
        PIR4bits.RC3IF= CLEAR; // Reset the ISR flag.
    } else if (PIR3bits.RC1IF) { // Interrupt on LORA RX pin
        Run_led = GLOW; // Led Indication for system in Operational Mode
        rxCharacter = rxByteLora(); // Read byte received at Reception Register
        // Check if any overrun occur due to continuous reception
        if (RC1STAbits.OERR) {
            RC1STAbits.CREN = 0;
            Nop();
            RC1STAbits.CREN = 1;
        }
        if (rxCharacter == '#') {
            msgIndex = CLEAR; // Reset message storage index to first character to start reading from '+'
            decodedString[msgIndex] = rxCharacter; // Load Received byte into storage buffer
            msgIndex++; // point to next location for storing next received byte
        } else if (msgIndex > 0 && msgIndex < 25) {
            decodedString[msgIndex] = rxCharacter; // Load received byte into storage buffer
            msgIndex++; // point to next location for storing next received byte
            // check if storage index is reached to last character of CMTI command
            if (rxCharacter == '$') {
                msgIndex = CLEAR;
                controllerCommandExecuted = true; // Set to indicate command received from lora	
            }
        }
        //SIM_led = DARK;  // Led Indication for LORA interrupt is done 
        PIR3bits.RC1IF= CLEAR; // Reset the ISR flag.
    } else if (PIR0bits.IOCIF) { //Interrupt-on-change pins
        Run_led = GLOW; // Led Indication for system in Operational Mode
        // Rising Edge -- All phase present
        if (IOCBF0 == 1) {
            __delay_ms(2500);__delay_ms(2500);
            if (Phase_Input == 0) {
                //phase is on
                IOCBF &= (IOCBF ^ 0xFF); //Clearing Interrupt Flags
                phaseFailureDetected = false;
                ////setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                __delay_ms(2500);__delay_ms(2500);
                RESET();
            } else {
                // phase is out
                IOCBF &= (IOCBF ^ 0xFF); //Clearing Interrupt Flags
                phaseFailureDetected = true; //true
                phaseFailureActionTaken = false;
            }
        }
        PIR0bits.IOCIF = CLEAR; // Reset the ISR flag.
    }          
}


/*************************************************************************************************************************

This function is called when an interrupt is occurred after 16 bit timer is overflow
The purpose of this interrupt handler is to count no. of overflows that the timer did.

 **************************************************************************************************************************/

void __interrupt(low_priority) timerInterrupt_handler(void) {
    /*To follow filtration  cycle sequence*/
    if (PIR0bits.TMR0IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        PIR0bits.TMR0IF = CLEAR;
        TMR0H = 0xE3; // Load Timer0 Register Higher Byte 
        TMR0L = 0xB0; // Load Timer0 Register Lower Byte
        Timer0Overflow++;
        // Control sleep count decrement for each one minute interrupt when Motor is ON i.e. Valve ON period 
        if (sleepCount > 0 && Irri_Motor == ON) {  // check additional condition of ValveDue
            sleepCount--;
            loraAliveCountCheck++; // increment for each sleep
            if (dryRunCheckCount == 0 || dryRunCheckCount < 3) {
                dryRunCheckCount++;
            }
            if (strncmp(decodedString+1, alive, 5) == 0 && strncmp(decodedString+6, slave, 5) == 0) {
                deleteDecodedString();
                loraAliveCount++;  //increment for each alive message               
            }
            if (loraAliveCountCheck <= loraAliveCount+1) { // check if alive count for each sleep is incremented
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("Lora is alive\r\n");
                //********Debug log#end**************//
                #endif
            }
            else {
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("Lora is not alive\r\n");
                //********Debug log#end**************//
                #endif
            }
        } 
		// Check Fertigation Level for each one minute interrupt when Fertigation Motor is ON during Valve ON period 
        //if (Fert_Motor == ON) {
        //    fertigationDry = false;
        //    if (!moistureSensorFailed) {  // to avoid repeated fertigation level check after sensor failure detected
        //        if (isFieldMoistureSensorWet(11)==false) {
        //            if (!moistureSensorFailed) { // to avoid sensor dry detection due to sensor failure
        //                Fert_Motor = OFF;
        //                fertigationDry = true;
        //            }
        //        }
        //    }
        //}
        //To follow fertigation cycle sequence
        if (Fert_Motor == ON) {
            if (Irri_Out9 == ON) {
                if(injector1OnPeriodCnt == injector1OnPeriod) {
                    Irri_Out9 = OFF;
                    injector1OnPeriodCnt = CLEAR;
                    injector1OffPeriodCnt++;
                    injector1CycleCnt++;
                }
                else injector1OnPeriodCnt++;
            }
            else if (Irri_Out9 == OFF) {
                if(injector1OffPeriodCnt == injector1OffPeriod) {
                    if (injector1CycleCnt < injector1Cycle) {
                        Irri_Out9 = ON;
                        injector1OnPeriodCnt++;
                        injector1OffPeriodCnt = CLEAR;
                    }
                    else injector1OffPeriodCnt = injector1OffPeriod + 1;
                }
                else injector1OffPeriodCnt++;
            }
            if (Irri_Out10 == ON) {
                if(injector2OnPeriodCnt == injector2OnPeriod) {
                    Irri_Out10 = OFF;
                    injector2OnPeriodCnt = CLEAR;
                    injector2OffPeriodCnt++;
                    injector2CycleCnt++;
                }
                else injector2OnPeriodCnt++;
            }
            else if (Irri_Out10 == OFF) {
                if(injector2OffPeriodCnt == injector2OffPeriod) {
                    if (injector2CycleCnt < injector2Cycle) {
                        Irri_Out10 = ON;
                        injector2OnPeriodCnt++;
                        injector2OffPeriodCnt = CLEAR;
                    }
                    else injector2OffPeriodCnt = injector2OffPeriod + 1;
                }
                else injector2OffPeriodCnt++;
            }
            if (Irri_Out11 == ON) {
                if(injector3OnPeriodCnt == injector3OnPeriod) {
                    Irri_Out11 = OFF;
                    injector3OnPeriodCnt = CLEAR;
                    injector3OffPeriodCnt++;
                    injector3CycleCnt++;
                }
                else injector3OnPeriodCnt++;
            }
            else if (Irri_Out11 == OFF) {
                if(injector3OffPeriodCnt == injector3OffPeriod) {
                    if (injector3CycleCnt < injector3Cycle) {
                        Irri_Out11 = ON;
                        injector3OnPeriodCnt++;
                        injector3OffPeriodCnt = CLEAR;
                    }
                    else injector3OffPeriodCnt = injector3OffPeriod + 1;
                }
                else injector3OffPeriodCnt++;
            }
            if (Irri_Out12 == ON) {
                if(injector4OnPeriodCnt == injector4OnPeriod) {
                    Irri_Out12 = OFF;
                    injector4OnPeriodCnt = CLEAR;
                    injector4OffPeriodCnt++;
                    injector4CycleCnt++;
                }
                else injector4OnPeriodCnt++;
            }
            else if (Irri_Out12 == OFF) {
                if(injector4OffPeriodCnt == injector4OffPeriod) {
                    if (injector4CycleCnt < injector4Cycle) {
                        Irri_Out12 = ON;
                        injector4OnPeriodCnt++;
                        injector4OffPeriodCnt = CLEAR;
                    }
                    else injector4OffPeriodCnt = injector4OffPeriod + 1;
                }
                else injector4OffPeriodCnt++;
            }
        }
        //*To follow filtration  cycle sequence*/
        if (filtrationCycleSequence == 99) {    // Filtration is disabled
            Timer0Overflow = 0;
        }
        else if (filtrationCycleSequence == 1 && Timer0Overflow == filtrationDelay1 ) { // Filtration1 Start Delay
                Timer0Overflow = 0;
                Filt_Out1 = ON;
                filtrationCycleSequence = 2;
        }
        else if (filtrationCycleSequence == 2 && Timer0Overflow == filtrationOnTime ) {  // Filtration1 On Period
            Timer0Overflow = 0;
            Filt_Out1 = OFF;
            filtrationCycleSequence = 3;
        }
        else if (filtrationCycleSequence == 3 && Timer0Overflow == filtrationDelay2 ) { // Filtration2 Start Delay
            Timer0Overflow = 0;
            Filt_Out2 = ON;
            filtrationCycleSequence = 4;
        }
        else if (filtrationCycleSequence == 4 && Timer0Overflow == filtrationOnTime ) { // Filtration2 On Period
            Timer0Overflow = 0;
            Filt_Out2 = OFF;
            filtrationCycleSequence = 5;
        }
        else if (filtrationCycleSequence == 5 && Timer0Overflow == filtrationDelay2 ) { // Filtration3 Start Delay
            Timer0Overflow = 0;
            Filt_Out3 = ON;
            filtrationCycleSequence = 6;
        }
        else if (filtrationCycleSequence == 6 && Timer0Overflow == filtrationOnTime ) { // Filtration3 On Period
            Timer0Overflow = 0;
            Filt_Out3 = OFF;
            filtrationCycleSequence = 7;
        }
        else if (filtrationCycleSequence == 7 && Timer0Overflow == filtrationSeperationTime ) { //Filtration Repeat Delay
            Timer0Overflow = 0;
            filtrationCycleSequence = 1;
        }
    }
/*To measure pulse width of moisture sensor output*/
    if (PIR5bits.TMR1IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        Timer1Overflow++;
        PIR5bits.TMR1IF = CLEAR;
    }

    if (PIR5bits.TMR3IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        PIR5bits.TMR3IF = CLEAR;
        TMR3H = 0xF0; // Load Timer3 Register Higher Byte 
        TMR3L = 0xDC; // Load Timer3 Register lower Byte 
        Timer3Overflow++;
        
        if (Timer3Overflow > timer3Count  && !controllerCommandExecuted) {
            controllerCommandExecuted = true; // Unlock key
            Timer3Overflow = 0;
            T3CONbits.TMR3ON = OFF; // Stop timer
            if (checkLoraConnection) {
                LoraConnectionFailed = true;
            }
			else if (checkMoistureSensor) {
                moistureSensorFailed = true;
            }
        } 
        else if (controllerCommandExecuted) {
            Timer3Overflow = 0;
            T3CONbits.TMR3ON= OFF; // Stop timer
        }       
    }
}
//**************interrupt service routine handler#end***********//



//****************************MAIN FUNCTION#Start***************************************//
 void main(void) {
    NOP();
    NOP();
    NOP();
    unsigned char last_Field_No = CLEAR;
    actionsOnSystemReset();
    while (1) {
nxtVlv: if (!valveDue && !phaseFailureDetected && !lowPhaseCurrentDetected) {
            LoraConnectionFailed = false;  // reset slave lora connection failed status for first  field
            wetSensor = false; // reset wet sensor for first wet field detection
            __delay_ms(50);
            scanValveScheduleAndGetSleepCount(); // get sleep count for next valve action
            __delay_ms(50);
            dueValveChecked = true;
        }
        if (valveDue && dueValveChecked) {
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("actionsOnDueValve_IN\r\n");
            //********Debug log#end**************//
            #endif
            dueValveChecked = false;
            actionsOnDueValve(iterator);// Copy field no. navigated through iterator. 
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("actionsOnDueValve_OUT\r\n");
            //********Debug log#end**************//
            #endif
        }
        // DeActivate last valve and switch off motor pump
        else if (valveExecuted) {
            LoraConnectionFailed = false;  // reset slave lora connection failed status for last  field
            wetSensor = false; // reset wet sensor for last wet field detection
            powerOffMotor();
            last_Field_No = readFieldIrrigationValveNoFromEeprom();
            deActivateValve(last_Field_No);      // Successful Deactivate valve
            valveExecuted = false;
            /***************************/
            sendSms(SmsMotor1, userMobileNo, noInfo); // Acknowledge user about successful action
            #ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            //setBCDdigit(0x05,0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
            #endif
            /***************************/
            startFieldNo = 0;
            //goto nxtVlv;
        }
        // system on hold
        if (onHold) {
            sleepCount = 0; // Skip Next sleep for performing hold operation
        }
        if(!LoraConnectionFailed && !wetSensor) {   //|| // Skip next block if Activate valve cmd is failed
            /****************************/
            deepSleep(); // sleep for given sleep count (	default/calculated )
            /****************************/
            // check if Sleep count executed with interrupt occurred due to new SMS command reception
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug((const char *)gsmResponse);
            transmitStringToDebug("\r\n");
            //********Debug log#end**************//
            #endif
            if (newSMSRcvd) {
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("newSMSRcvd_IN\r\n");
                //********Debug log#end**************//
                #endif
                //setBCDdigit(0x02,1); // "2" BCD indication for New SMS Received 
                newSMSRcvd = false; // received command is processed										
                extractReceivedSms(); // Read received SMS
                //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                //__delay_ms(500);
                deleteMsgFromSIMStorage();
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("newSMSRcvd_OUT\r\n");
                //********Debug log#end**************//
                #endif
            } 
            //check if Sleep count executed without external interrupt
            else {
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSleepCountFinish_IN\r\n");
                //********Debug log#end**************//
                #endif
                actionsOnSleepCountFinish();
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSleepCountFinish_OUT\r\n");
                //********Debug log#end**************//
                #endif
                if ( !rtcBatteryLevelChecked) {
                    if (isRTCBatteryDrained()){
                        /***************************/
                        sendSms(SmsRTC1, userMobileNo, noInfo); // Acknowledge user about replace RTC battery
                        #ifdef SMS_DELIVERY_REPORT_ON_H
                        sleepCount = 2; // Load sleep count for SMS transmission action
                        sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                        //setBCDdigit(0x05,0);
                        deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                        //setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                        #endif
                        /***************************/
                    }
                }
            }
        }
    }
}
//****************************MAIN FUNCTION#End***************************************//
 /*Features Implemented*/
 // On hold fertigation and irrigation
 // Restart after phase/dry run detection -- check flow
 // Phase failure during sleep -- Done
 // filtration done but need to check flow -- Done
 // fertigation status eeprom read write -- Done
 // EEprom address need to be mapped -- Done
 // fertigation interrupt due to power and dry run -- Done
 // eeprom read write for fertigation values -- Done
 // save status in eeprom -- Done
 // Activate filtration and disabling message reception -- Done
 // Activate filtration and disabling  -- Done
 // eeprom read write for filtration values -- Done
 // Sleep count correction for next due valve. -- Done
 // Send Filtration status -- Done
 // actionsOnSystemReset
 // Sleep count correction for valve without filtration
 // Sleep count controlled by Timer0 during valve in action
 // phase failure at system start
 // Decision on valve interrupted due to dry run.
 
 
 
/*features remaining*/
 // RTC battery check -- unDone
 // RTC battery threshold not set
 // CT threshold not set
 // Separating fertigation cycle
 // Code Protection
 // Sensor calibration
 // cycles for field execution in dry run and phase failure
 /************High Priority**************/
 // due in hours
 // Onhold sms verify
 // random factory password
 // gsm get time
 
 
 /*Changes in New Board*/
 // RYB phase detection logic reversed i.e. 1 for no phase and 0 for phase
 // Rain Sensor 7 and 8 pin changed from RC0 and RC1 to RE4 and RD4 respectively
 // Added Priority and changed SMS format
 // decode of incoming sms required in extract receive function
 
             
