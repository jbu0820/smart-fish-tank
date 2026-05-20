#include "i2c_lcd.h"

void I2C_Init(void) {
    TWSR = 0x00;
    TWBR = 72;
    TWCR = (1 << TWEN);
}

void I2C_Start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Write(unsigned char data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void I2C_Stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void LCD_WriteNibble(unsigned char nibble, unsigned char rs_bit) {
    I2C_Start();
    I2C_Write(LCD_ADDR);
    I2C_Write(nibble | rs_bit | BACKLIGHT | EN);
    _delay_us(1);
    I2C_Write(nibble | rs_bit | BACKLIGHT);
    _delay_us(50);
    I2C_Stop();
}

void LCD_SendCommand(unsigned char cmd) {
    LCD_WriteNibble(cmd & 0xF0, 0);
    LCD_WriteNibble((cmd << 4) & 0xF0, 0);
}

void LCD_SendData(unsigned char data) {
    LCD_WriteNibble(data & 0xF0, RS);
    LCD_WriteNibble((data << 4) & 0xF0, RS);
}

void LCD_Init(void) {
    _delay_ms(100);           /* 전원 안정화 — 50ms → 100ms */

    LCD_SendCommand(0x33);
    _delay_ms(5);             /* ← 추가 */

    LCD_SendCommand(0x32);
    _delay_ms(5);             /* ← 추가 */

    LCD_SendCommand(0x28);
    _delay_ms(2);             /* ← 추가 */

    LCD_SendCommand(0x0C);
    _delay_ms(2);             /* ← 추가 */

    LCD_SendCommand(0x06);
    _delay_ms(2);             /* ← 추가 */

    LCD_Clear();
    _delay_ms(5);             /* ← 추가 */
}
void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    _delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_SendCommand(addr);
}

void LCD_WriteString(char *str) {
    while (*str) LCD_SendData(*str++);
}