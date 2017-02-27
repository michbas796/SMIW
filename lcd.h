#ifndef LCD_H_
#define LCD_H_

#define F_CPU 1000000UL //Zegar 1 MHz
#include <avr/io.h>
#include <util/delay.h>

#define setRS() 	PORTA |= 0b00010000
#define clearRS() 	PORTA &= 0b11101111

#define setRW() 	PORTA |= 0b00100000
#define clearRW() 	PORTA &= 0b11011111

#define setE()		PORTA |= 0b01000000
#define clearE()	PORTA &= 0b10111111

#define clearLcd()	sendToLcd(0x01, instruction);

typedef enum DataType {instruction = 0, data = 1} lcd_data_type;

unsigned char lcd_busy();
void sendToLcd(unsigned char data, lcd_data_type dataType);
void sendTextToLcd(char * text);
void initLcd();

#endif /* LCD_H_ */