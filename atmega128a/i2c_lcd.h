#ifndef I2C_LCD_H_
#define I2C_LCD_H_

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define LCD_ADDR 0x4E // 0x27 << 1

#define RS 0x01
#define EN 0x04
#define BACKLIGHT 0x08

void I2C_Init(void);
void LCD_Init(void);
void LCD_SendCommand(unsigned char cmd);
void LCD_SendData(unsigned char data);
void LCD_WriteString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);

#endif