/*
 * File name            : i2c_LCD_PCF8574.c
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
#include "i2c_LCD_PCF8574.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif

#define LCD_CLEAR_DISPLAY           0x01    // Mode : Clears display

#define LCD_RETURN_HOME             0x02    // Mode : Returns cursor to home posn.

// Entry Mode Set
#define LCD_ENTRY_MODE_SET          0x04        // Mode : Entry Mode Set, Sets the cursor move dir and specs whether or not to shift the display
#define LCD_INCREMENT               0x02        // Sub Mode of ENTRY_MODE_SET : Increment DDRAM (I/D), Entry Left
#define LCD_DECREMENT               0x00        // Sub Mode of ENTRY_MODE_SET : Decrement DDRAM (I/D), Entry Right
#define LCD_SHIFT_ON                0x01        // Sub Mode of ENTRY_MODE_SET : Shift On  (S), Shift Display when byte written. Display Shift
#define LCD_SHIFT_OFF               0x00        // Sub Mode of ENTRY_MODE_SET : Shift Off (S), Don't shift display when byte written. Cursor Move

// Display Function
#define LCD_DISPLAY_ON_OFF          0x08    // Mode : Display On/Off, Sets on/off of all display, Cursor on/off, Cursor Blink on/off
#define LCD_DISPLAY_ON              0x04        // Sub Mode of DISPLAY_ON_OFF : Puts display on  (D)
#define LCD_DISPLAY_OFF             0x00        // Sub Mode of DISPLAY_ON_OFF : Puts display off (D)
#define LCD_CURSOR_ON               0x02        // Sub Mode of DISPLAY_ON_OFF : Puts cursor on   (C)
#define LCD_CURSOR_OFF              0x00        // Sub Mode of DISPLAY_ON_OFF : Puts cursor off  (C)
#define LCD_BLINKING_ON             0x01        // Sub Mode of DISPLAY_ON_OFF : Blinking cursor  (B)
#define LCD_BLINKING_OFF            0x00        // Sub Mode of DISPLAY_ON_OFF : Solid cursor     (B)

// Display Control
#define LCD_MV_CUR_SHIFT_DISPLAY    0x10    // Mode : Move the cursor and shifts the display
#define LCD_DISPLAY_SHIFT           0x08        // Sub Mode of CURSOR_SHFT_DIS : Display shifts after char print   (SC)
#define LCD_CURSOR_SHIFT            0x00        // Sub Mode of CURSOR_SHFT_DIS : Cursor shifts after char print    (SC)
#define LCD_SHIFT_RIGHT             0x04        // Sub Mode of CURSOR_SHFT_DIS : Cursor or Display shifts to right (RL)
#define LCD_SHIFT_LEFT              0x00        // Sub Mode of CURSOR_SHFT_DIS : Cursor or Display shifts to left  (RL)

// Function Set
#define LCD_FUNCTION_SET            0x20    // Mode : Set the type of interface that the display will use
#define LCD_INTF8BITS               0x10        // Sub Mode of FUNCTION_SET : Select 8 bit interface         (DL)
#define LCD_INTF4BITS               0x00        // Sub Mode of FUNCTION_SET : Select 4 bit interface         (DL)
#define LCD_TWO_LINES               0x08        // Sub Mode of FUNCTION_SET : Selects two char line display  (N)
#define LCD_ONE_LINE                0x00        // Sub Mode of FUNCTION_SET : Selects one char line display  (N)
#define LCD_FONT_5_10               0x04        // Sub Mode of FUNCTION_SET : Selects 5 x 10 Dot Matrix Font (F)
#define LCD_FONT_5_7                0x00        // Sub Mode of FUNCTION_SET : Selects 5 x 7 Dot Matrix Font  (F)

#define LCD_CG_RAM_ADDRESS          0x40        // Mode : Enables the setting of the Char Gen (CG) Ram Address, to be or'ed with require address
#define LCD_CG_RAM_ADDRESS_MASK     0b00111111  // Used to mask off the lower 6 bits of valid CG Ram Addresses

#define LCD_DD_RAM_ADDRESS          0x80        // Mode : Enables the setting of the Display Data (DD) Ram Address, to be or'ed with require address
#define LCD_DD_RAM_ADDRESS_MASK     0b01111111    // Used to mask off the lower 6 bits of valid DD Ram Addresses

//#define USE_BUSY_FLAG               // Define this if you wish to use busy flag polling on slow LCD activities

// Change here for your I2C to 16 pin parallel interface // TODO Adapt
#define Bl 0b00001000  // lcdBacklight enable bit (On = 1, Off =0)
#define En 0b00000100  // Enable bit (Enable on low edge)
#define Rw 0b00000010  // Read/Write bit (Read = 1, Write = 0)
#define Rs 0b00000001  // Register select bit (Data = 1, Control = 0)

// Change here for your I2C to 16 pin parallel interface // TODO Adapt
#define LCD_INIT      ((0b00000000 | En) & ~Rs) & (~Rw) // Used to set all the O/Ps on the PCF8574 to initialise the LCD
#define LCD_8BIT_INIT 0b00110000 // Used to initialise the interface at the LCD
#define LCD_4BIT_INIT 0b00100000 // Used to initialise the interface at the LCD

#define LCD_PCF8574_ADDR         (0x4E)  // Modify this if the default address is altered 
#define LCD_PCF8574_WEAK_PU      0b11110000 // Used to turn on PCF8574 Bits 7-4 on. To allow for read of LCD.

#define LCD_BUSY_FLAG_MASK       0b10000000 // Used to mask off the status of the busy flag
#define LCD_ADDRESS_COUNTER_MASK 0b01111111 // Used to mask off the value of the Address Counter
#define LCD_MAX_COLS             20
#define LCD_MAX_ROWS             4

//
// Code was written with the following assumptions as to PCF8574 -> Parallel 4bit convertor interconnections
// controlling a 20 by 4 LCD display. Assumes A0...A2 on PCF8574 are all pulled high. Giving address of 0x4E or 0b01001110 (0x27)
// (the last bit [b0] is I2C R/nW bit)
//
// Pin out for LCD display (16 pins)
// ---------------------------------
// 1  - Gnd
// 2  - Vcc
// 3  - VContrast
// 4  - RS - P0 - Pin 4 PCF8574
// 5  - RW - P1 - Pin 5 PCF8574
// 6  - En - P2 - Pin 6 PCF8574
// 7  - D0 - Don't Care
// 8  - D1 - Don't Care
// 9  - D2 - Don't Care
// 10 - D3 - Don't Care
// 11 - D4 - P4 - Pin 9  PCF8574
// 12 - D6 - P5 - Pin 10 PCF8574
// 13 - D6 - P6 - Pin 11 PCF8574
// 14 - D7 - P7 - Pin 12 PCF8574
// 15 - Anode   LED
// 16 - Cathode LED
//
// PCF8574 register and pin mapping
// Bit 0 - RS  - P0 - Pin 4  PCF8574
// Bit 1 - RW  - P1 - Pin 5  PCF8574
// Bit 2 - En  - P2 - Pin 6  PCF8574
// Bit 3 - Led - P3 - Pin 7  PCF8574 (Active High, Led turned on)
// Bit 4 - D4  - P4 - Pin 9  PCF8574
// Bit 5 - D5  - P5 - Pin 10 PCF8574
// Bit 6 - D6  - P6 - Pin 11 PCF8574
// Bit 7 - D7  - P7 - Pin 12 PCF8574
//



// The display is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 4-bit interface data
//    N = 0; 2-line display
//    F = 0; 5x7 dot character font
// 3. Display on/off control:
//    D = 1; Display on
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//

#define LCD_LINE1	0x00		// Constant used to point to start of LCD Line 1
#define LCD_LINE2	0x40		// Constant used to point to start of LCD Line 2
#define LCD_LINE3	0x14		// Constant used to point to start of LCD Line 3
#define LCD_LINE4	0x54		// Constant used to point to start of LCD Line 4

// DDRAM address:
// Display position
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
// 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13
// 40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F 50 51 52 53
// 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27
// 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F 60 61 62 63 64 65 66 67


// Local variable declarations

static unsigned char _functionset = 0;
static unsigned char _entrymodeset = 0;
static unsigned char _displayfunction = 0;
static unsigned char _displaycontrol = 0;
static unsigned char _numlines = 0;
static unsigned char _lcdBacklightval = 0;


// Local function declarations

static void lcdSend(unsigned char value, unsigned char mode);
static unsigned char LCDreceive(unsigned char RsMode);
static void lcdWrite4Bits(unsigned char value);
static void LCDpulseEnableNeg(unsigned char value);
static void LCDpulseEnablePos(unsigned char value);
static void LCDwritePCF8574(unsigned char value);
//static unsigned char LCDreadPCF8574(void);


void lcdInit(void) {
    //_lcdBacklightval &= ~Bl; // Off at start up
    _lcdBacklightval |= Bl; // On at start up
    _numlines = LCD_MAX_ROWS;

    // Ensure supply rails are up before config sequence
    __delay_ms(50);

    // Set all control and data lines low. D4 - D7, En (High=1), Rw (Low = 0 or Write), Rs (Control/Instruction) (Low = 0 or Control)
    lcd_i2cWriteByteSingleReg(LCD_PCF8574_ADDR, LCD_INIT); // lcdBacklight off (Bit 3 = 0)
    __delay_us(100);
    // Sequence to put the LCD into 4 bit mode this is according to the hitachi HD44780 datasheet page 109

    // we start in 8bit mode
    lcdWrite4Bits(LCD_8BIT_INIT);
    __delay_us(4500);  // wait more than 4.1ms

    // second write
    lcdWrite4Bits(LCD_8BIT_INIT);
    __delay_us(150); // wait > 100us

    // third write
    lcdWrite4Bits(LCD_8BIT_INIT);
    __delay_us(150);

    // now set to 4-bit interface
    lcdWrite4Bits(LCD_4BIT_INIT);
    __delay_us(150);

    // set # lines, font size, etc.
    _functionset = LCD_INTF4BITS | LCD_TWO_LINES | LCD_FONT_5_7;
    lcdCommandWrite(LCD_FUNCTION_SET | _functionset);
    //DelayMicroseconds(150);

    _displayfunction = LCD_DISPLAY_OFF | LCD_CURSOR_OFF | LCD_BLINKING_OFF;
    lcdDisplayOff();

    // turn the display on with no cursor or blinking default
    lcdDisplayOn();

    // set the entry mode
    _entrymodeset = LCD_INCREMENT | LCD_SHIFT_OFF; // Initialize to default text direction (for roman languages)
    lcdCommandWrite(LCD_ENTRY_MODE_SET | _entrymodeset);

    // Display Function set
    // _displayfunction = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINKING_OFF;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);

    // Display Control set
    _displaycontrol = LCD_DISPLAY_SHIFT | LCD_SHIFT_LEFT;
    lcdCommandWrite(LCD_MV_CUR_SHIFT_DISPLAY | _displaycontrol);

    // clear display and return cursor to home position. (Address 0)
    lcdClear();
}

/********** high level commands, for the user! */


void lcdWriteChar(unsigned char message) {
    lcdDataWrite(message);
}

void lcdWriteString(const char *message) {
    while (*message)
        lcdDataWrite(*message++);
}

void lcdWriteStringAtCenter(const char *message, unsigned char row) {
    unsigned char col;
    for(col=0; message[col]!='\0'; col++);
    col = (LCD_MAX_COLS-col);
    col = col/2;
    lcdSetCursor(row,col);
    while (*message)
        lcdDataWrite(*message++);
}


void lcdClear(void) {
    lcdCommandWrite(LCD_CLEAR_DISPLAY);// clear display, set cursor position to zero
#ifdef USE_BUSY_FLAG
    while (LCDbusy()){};
#else
    __delay_ms(30); // this command takes a long time!
#endif
}

void LCDhome(void) {
    lcdCommandWrite(LCD_RETURN_HOME);  // set cursor position to zero
#ifdef USE_BUSY_FLAG
    while (LCDbusy()){};
#else
    __delay_ms(30);  // this command takes a long time!
#endif
}

void lcdSetCursor(unsigned char row, unsigned char col) {
  unsigned char row_offsets[4] = { LCD_LINE1, LCD_LINE2, LCD_LINE3, LCD_LINE4 };
  if ( row > _numlines ) {
    row = _numlines-1;    // we count rows starting w/0
  }

  lcdCommandWrite(LCD_DD_RAM_ADDRESS | (col + row_offsets[row-1]));
}

// Turn the display on/off (quickly)
void lcdDisplayOff(void) {
    _displayfunction &= ~LCD_DISPLAY_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}

void lcdDisplayOn(void) {
    _displayfunction |= LCD_DISPLAY_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}

// Turns the underline cursor on/off
void lcdCursorOff(void) {
    _displayfunction &= ~LCD_CURSOR_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}

void lcdCursorOn(void) {
    _displayfunction |= LCD_CURSOR_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}

// Turn on and off the blinking cursor
void lcdBlinkOff(void) {
    _displayfunction &= ~LCD_BLINKING_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}

void lcdBlinkOn(void) {
    _displayfunction |= LCD_BLINKING_ON;
    lcdCommandWrite(LCD_DISPLAY_ON_OFF | _displayfunction);
}
// These commands scroll the display without changing the RAM
void lcdScrollDisplayLeft(void) {
    _displaycontrol &=  ~LCD_SHIFT_RIGHT;
    _displaycontrol |=   LCD_DISPLAY_SHIFT;
    lcdCommandWrite(LCD_MV_CUR_SHIFT_DISPLAY | _displaycontrol);
}

void lcdScrollDisplayRight(void) {
    _displaycontrol |=  LCD_SHIFT_RIGHT;
    _displaycontrol |=  LCD_DISPLAY_SHIFT;
    lcdCommandWrite(LCD_MV_CUR_SHIFT_DISPLAY | _displaycontrol);
}


// This is for text that flows Left to Right
void lcdLeftToRight(void) {
    _entrymodeset |= LCD_INCREMENT;
    //_entrymodeset |= LCD_SHIFT_ON;
    lcdCommandWrite(LCD_ENTRY_MODE_SET | _entrymodeset);
}

// This is for text that flows Right to Left
void lcdRightToLeft(void) {
    _entrymodeset &= ~LCD_INCREMENT;
    //_entrymodeset &= ~LCD_SHIFT_ON;
    lcdCommandWrite(LCD_ENTRY_MODE_SET | _entrymodeset);
}

// This will 'right justify' text from the cursor. Display shift
void lcdAutoscroll(void) {
    _entrymodeset |= LCD_SHIFT_ON;
    //_entrymodeset |= LCD_INCREMENT;
    lcdCommandWrite(LCD_ENTRY_MODE_SET | _entrymodeset);
}

// This will 'left justify' text from the cursor. Cursor Move
void lcdNoAutoscroll(void) {
    _entrymodeset &= ~LCD_SHIFT_ON;
    //_entrymodeset &= ~LCD_INCREMENT;
    lcdCommandWrite(LCD_ENTRY_MODE_SET | _entrymodeset);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcdCreateChar(unsigned char location, unsigned char charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    lcdCommandWrite(LCD_CG_RAM_ADDRESS | (location << 3));
    for (int i=0; i<8; i++)
        lcdDataWrite(charmap[i]);
}

// Turn the (optional) lcdBacklight off/on
void lcdNoBacklight(void) { 
    _lcdBacklightval &= ~Bl;
    //LCDwritePCF8574(LCDreadPCF8574());  // Dummy write to LCD, only led control bit is of interest
}

void lcdBacklight(void) { 
    _lcdBacklightval |= Bl;
    //LCDwritePCF8574(LCDreadPCF8574());  // Dummy write to LCD, only led control bit is of interest
}


/*********** mid level commands, for sending data/cmds */

inline void lcdCommandWrite(unsigned char value) {
    lcdSend(value, Rs & ~Rs);
}

inline void lcdDataWrite(unsigned char value) {
    lcdSend(value, Rs);
}

/************ low level data write commands **********/

// Change this routine for your I2C to 16 pin parallel interface, if your pin interconnects are different to that outlined above // TODO Adapt

// write either command or data
static void lcdSend(unsigned char value, unsigned char RsMode) {
    unsigned char highnib = value & 0xF0;

    unsigned char lownib  = value << 4;
    lownib &= 0xF0;

    lcdWrite4Bits((highnib) | En | RsMode);
    lcdWrite4Bits((lownib ) | En | RsMode);
}

// Change this routine for your I2C to 16 pin parallel interface, if your pin interconnects are different to that outlined above // TODO Adapt

static void lcdWrite4Bits(unsigned char nibEnRsMode) {
    LCDwritePCF8574(nibEnRsMode & ~Rw);
    LCDpulseEnableNeg(nibEnRsMode & ~Rw);
}

static void LCDpulseEnableNeg(unsigned char _data) {
    LCDwritePCF8574(_data | En);	// En high
    __delay_us(1);		// enable pulse must be >450ns

    LCDwritePCF8574(_data & ~En);	// En low
    __delay_us(50);		// commands need > 37us to settle
}

static void LCDpulseEnablePos(unsigned char _data) {
    LCDwritePCF8574(_data & ~En);	// En low
    __delay_us(1);		// enable pulse must be >450ns

    LCDwritePCF8574(_data | En);	// En high
    __delay_us(50);		// commands need > 37us to settle
}


static void LCDwritePCF8574(unsigned char value) {
    lcd_i2cWriteByteSingleReg(LCD_PCF8574_ADDR, value | _lcdBacklightval);
}



//miscellaneous Functions

 void exerciseDisplay(void) {
    //lcdDisplayScrolling("Bhoomi Jalasandharan");
    //autoIncrement();
    
    //lcdDisplayNoScrolling("Bhoomi Jalasandharan Udyamii LLP");
    lcdDisplayLeftScroll("Lora controller");
    lcdDisplayRightScroll("");
    //displayOnOff();
    //lcdBacklightControl();
    //cursorControl();
    // */
}
 
void lcdDisplayScrolling(const char *message) {
    //unsigned char *message_ptr = (unsigned char *) message;
    lcdClear();
    lcdCursorOff();
    lcdBlinkOff();
    lcdAutoscroll();
    lcdSetCursor(0,16);
    while (*message)
    {
        lcdWriteChar((char) *message++);
        __delay_ms(400);
    }
}

void lcdDisplayNoScrolling(const char *message) {
    unsigned char *message_ptr = (unsigned char *) message;
    lcdClear();
    lcdNoAutoscroll();
    lcdSetCursor(0,0);
    while (*message_ptr)
    {
        lcdWriteChar((char) *message_ptr++);
        __delay_ms(400);
    }
}


void lcdDisplayLeftScroll(const char *message) {
    lcdClear();
    lcdCursorOff();
    lcdBlinkOff();
    lcdSetCursor(0,0);
    //lcdWriteString(*message);
    lcdSetCursor(1,16);
    lcdWriteString("<----");
    for (unsigned char x = 0; x< 8; x++)
    {
        lcdScrollDisplayLeft();
        __delay_ms(500);
    }
}

void lcdDisplayRightScroll(const char *message) {
    lcdClear();
    lcdSetCursor(0,8);
    //lcdWriteString(*message);
    lcdSetCursor(0,58);
    lcdWriteString("---->");
    for (unsigned char x = 0; x< 8; x++)
    {
        lcdScrollDisplayRight();
        __delay_ms(500);
    }
}



void displayOnOff(void) {
    lcdClear();
    lcdCursorOff();
    lcdBlinkOff();
    for (unsigned char x = 0; x < 6; x++) {
        if (x%2) {
            LCDhome();
            lcdWriteString("           ");
            LCDhome();
            lcdWriteString("Display On ");
            lcdDisplayOn();
        } else {
            lcdSetCursor(0,0);
            lcdWriteString("           ");
            lcdSetCursor(0,0);
            lcdWriteString("Display Off");
            lcdDisplayOff();
        }
        __delay_ms(750);
    }
}

void lcdBacklightControl(void) {
    lcdClear();
    lcdCursorOff();
    lcdBacklight();
    lcdWriteString("lcdBacklight On");
    __delay_ms(1000);
    lcdClear();
    //lcdSetCursor(0,0);
    lcdNoBacklight();
    lcdWriteString("lcdBacklight Off");
    __delay_ms(1000);
    lcdClear();
    //lcdSetCursor(0,0);
    lcdBacklight();
    lcdWriteString("lcdBacklight On");
    __delay_ms(1000);
    for (unsigned char x = 0; x < 6; x++) {
        if (x%2) {
            lcdSetCursor(0,0);
            lcdWriteString("             ");
            lcdSetCursor(0,0);
            lcdWriteString("lcdBacklight On ");
            lcdBacklight();
        } else {
            lcdClear();
            lcdWriteString("lcdBacklight Off");
            lcdNoBacklight();
        }
        __delay_ms(750);
    }
}

void printAt(void) {
    lcdClear();
    lcdCursorOff();
    //lcdSetCursor(0,0);
    lcdWriteString("@:0,0");
    lcdSetCursor(1,1);
    lcdWriteString("@:1,1");
    lcdSetCursor(2,2);
    lcdWriteString("@:2,2");
    lcdSetCursor(3,3);
    lcdWriteString("@:3,3");
    lcdSetCursor(0,12);
    lcdWriteString("Print at");
    __delay_ms(3000);
}


void cursorControl(void) {
    lcdClear();
    lcdCursorOff();
    lcdWriteString("Cursor Off");
    __delay_ms(1500);
    lcdClear();
    lcdCursorOn();
    lcdWriteString("Cursor On");
    __delay_ms(1500);
    lcdClear();
    lcdBlinkOn();
    lcdWriteString("Blink On");
    __delay_ms(1500);
    lcdClear();
    lcdBlinkOff();
    lcdWriteString("Blink Off");
    __delay_ms(1500);
    lcdClear();
    lcdWriteString("Cursor Home");
    LCDhome();
    __delay_ms(1500);
    lcdClear();
    lcdWriteString("Cursor Home & Blink");
    LCDhome();
    lcdBlinkOn();
    __delay_ms(3000);
}

void autoIncrement(void)
{
    char Autoscroll[] = "Autoscroll Autoscroll Autoscroll Autoscroll";
    char NoAutoscroll[] = "No Autoscroll";
    unsigned char * p;

    lcdClear();
    lcdCursorOff();
    lcdBlinkOff();
    lcdAutoscroll();
    //p = Autoscroll;
    lcdSetCursor(1,15);
    while (*p)
    {
        lcdWriteChar((char) *p++);
        __delay_ms(400);
    }
    for (int i = 0; i<2; i++) {
        __delay_ms(3000);
    } // 6 seconds delay

    lcdClear();
    lcdNoAutoscroll();
    //p = NoAutoscroll;
    lcdSetCursor(1,3);
    while (*p)
    {
        lcdWriteChar((char) *p++);
        __delay_ms(400);
    }
    for (int i = 0; i<2; i++) {
        __delay_ms(3000);
    } // 6 seconds delay
}

 

