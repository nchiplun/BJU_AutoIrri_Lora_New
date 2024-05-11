#include "congfigBits.h"
#include "I2C_UTILS.h"
#include "LCD_PCF8574.h"
#include "variableDefinitions.h"



#define I2C_WRITE   0b11111110
#define I2C_READ    0b00000001

//SSPADD Calc : Baud = F_OSC/(4 * (SSPADD + 1))




void lcd_i2cWriteByteSingleReg(unsigned char device, unsigned char info)
{
    lcd_i2cWait();
    lcd_i2cStart();
    lcd_i2cWrite(device & I2C_WRITE);
    lcd_i2cWait();
    lcd_i2cWrite(info);
    lcd_i2cStop();
}



