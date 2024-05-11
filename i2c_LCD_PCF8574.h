/*
 * File name            : i2c_LCD_PCF8574.h
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : RTC_DS1307 functions header file
 */

#ifndef I2C_LCD_PCF8574_H
#define	I2C_LCD_PCF8574_H


//-----------[ Functions' Prototypes ]--------------

//---[ LCD Routines ]---
void lcdInit(void);
void lcdWriteChar(unsigned char message);
void lcdWriteString(const char *message);
void lcdWriteStringAtCenter(const char *message, unsigned char row);

void lcdClear(void);
void LCDhome(void);

void lcdDisplayOff(void);
void lcdDisplayOn(void);
void lcdBlinkOff(void);
void lcdBlinkOn(void);
void lcdCursorOff(void);
void lcdCursorOn(void);
void lcdScrollDisplayLeft(void);
void lcdScrollDisplayRight(void);
void lcdLeftToRight(void);
void lcdRightToLeft(void);
void lcdNoBacklight(void);
void lcdBacklight(void);
void lcdAutoscroll(void);
void lcdNoAutoscroll(void);
void lcdCreateChar(unsigned char location, unsigned char charmap[]);
void lcdSetCursor(unsigned char row, unsigned char col);

inline void lcdCommandWrite(unsigned char value);
inline void lcdDataWrite(unsigned char value);

void exerciseDisplay(void);
void lcdDisplayLeftScroll(const char *);
void lcdDisplayRightScroll(const char *);
void lcdDisplayScrolling(const char *);
void lcdDisplayNoScrolling(const char *);
void displayOnOff(void);
void lcdBacklightControl(void);
void cursorControl(void);
void autoIncrement(void);


#endif
 /* I2C_LCD_PCF8574_H */

