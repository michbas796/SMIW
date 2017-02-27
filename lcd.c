#include "lcd.h"

/*

*/
unsigned char lcd_busy()
{
	unsigned char flag;
	clearRS();
	setRW();
	DDRA &= 0b11110111;
	setE();
	_delay_us(1);
	flag = PINA & 0x08;
	clearE();
	setE();
	_delay_us(1);
	clearE();
	DDRA |= 0b00001000;
	_delay_us(1);
	return flag;
}

/*
Funkcja wysyłające dane do wyświetlacza LCD.
Parametry:
	data - dane do wysłania
	dataType - określa typ danych, możliwe wartości:
		- instruction - rozkaz
		- data - dane do wyświetlenia
*/
void sendToLcd(unsigned char data, lcd_data_type dataType)
{
	while (lcd_busy());

	clearRW();
	if (dataType == instruction)
	clearRS();
	else
	setRS();
	
	PORTA =(PORTA & 0xF0)|((data & 0xF0) >> 4); //wysłanie czterech starszych bitów
	setE();
	_delay_us(2);
	clearE();
	_delay_us(2);
	
	PORTA = (PORTA & 0xF0)|(data & 0x0F);   //wysłanie czterech młodszych bitów
	setE();
	_delay_us(2);
	clearE();
	_delay_us(2);
}

/*
Funkcja wysyłająca tekst do wyświetlenia na wyświetlaczu.
Parametry:
	text - tekst do wyświetlenia
*/
void sendTextToLcd(char * text)
{
	unsigned char i = 0;
	while (text[i])
	{
		sendToLcd(text[i++], data);
		_delay_us(10);
	}
}

/*
Funkcja inicjalizująca wyświetlacz.
*/
void initLcd()
{

	//Wyświetlacz LCD jest podłączony do portu A
	//PORTA0 - PORTA3 - linia danych DB4 - DB7
	//PORTA4 - RS
	//PORTA5 - R/W
	//PORTA6 - E

	int i;

	_delay_ms(50);
	DDRA = 0xFF;
	clearRW();
	clearRS();
	_delay_ms(10);

	for (i = 0; i < 3; i++)
	{
		setE();
		//wysłanie 3 do wyświetlacza
		PORTA=(PORTA & 0xF0)|0x03;
		_delay_ms(1);
		clearE();
		_delay_ms(1);
	}

	setE();
	PORTA=(PORTA & 0xF0)|0x02;  // wysłanie 2- do LCD
	_delay_ms(1);
	clearE();
	sendToLcd(0x28, instruction);  	// interfejs 4 bitowy, 2 linie
	sendToLcd(0x08, instruction);	//wyłącz LCD, wyłącz kursor, wyłącz mruganie 00001000
	sendToLcd(0x01, instruction);	//czysc LCD
	sendToLcd(0x06, instruction);	//zwiekszanie pozycji kursora po każdy  znaku
	sendToLcd(0x0c, instruction);	//wlacz wyswietlacz bez kursora
	_delay_ms(1);
}
