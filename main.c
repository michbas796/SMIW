#define F_CPU 1000000UL //Zegar 1 MHz
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "ff.h"		/* Declarations of FatFs API */
#include "diskio.h"

FIL Fil;			/* File object needed for each open file */

#include "i2c.h"
#include "lcd.h"

#define HOUR 0
#define MINUTE 1
#define SECOND 2
#define DAY 3
#define MONTH 4
#define YEAR 5

#define RTC_ADDR 0xA2
#define HTU21D_ADDR 0x80
	
void readTime(unsigned char * hours, unsigned char * minutes, unsigned char * seconds, unsigned char * day, unsigned char * month, unsigned char * year);
void printTime(unsigned char hour, unsigned char minute, unsigned char second, unsigned char day, unsigned char month, unsigned char year);
void setStartDate();
void setTime(unsigned char hour, unsigned char minute, unsigned char second, unsigned char day, unsigned char month, unsigned char yer);		
void getTime(char * timeString);
unsigned char toBCD(unsigned char value);
void writeToSDCard(double minTemperature, double maxTemperature, double minHumidity, double maxHumidity, char * minTempTime, char * maxTempTime, char * minHumTime, char * maxHumTime);
void setFileTime(char * fileName);



int main(void)
{	
	unsigned sensorTempValue = 0;
	unsigned sensorHumValue = 0;
	unsigned char LSB = 0;
	unsigned char MSB = 0;
	double temperature = 0;
	double humidity = 0;
	char tempString[10];
	char humString[10];	
	unsigned char seconds;
	unsigned char minutes;
	unsigned char hours;
	unsigned char day;
	unsigned char month;
	unsigned char year;
	double minTemperature;
	double maxTemperature;
	double minHumidity;
	double maxHumidity;
	char minTempTime[20];
	char maxTempTime[20];
	char minHumTime[20];
	char maxHumTime[20];
	
	
	//konfiguracja portów:	
		
	DDRC = 0xC0; 
	PORTC = 0xC0;
	
	GICR = 1<<INT0;	 // Enable INT0
	MCUCR = 0x00;
	
	DDRD = 0x0C;		// Set PD2 as input (Using for interupt INT0)
	PORTD = 0x0C;		// Enable PD2 pull-up resistor	
	
	//icjalizacja wyświetlacza:
	initLcd();
	
	wdt_enable(WDTO_1S);
	
	//inicjalizacja i2c:
	TWSR = 0x00;   // Select Prescaler of 1
	//częstotliwość sygnału SCL = 11059200 / (16 + 2 * 48 * 1) = 98.743 khz
	TWBR = 0x30;  
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x02);
	seconds = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	if ((seconds & 0x80) == 0x80) {
		setStartDate();
	}
	
	//wysłanie komendy odczytu temperatury
	cli();
	i2c_start();
	i2c_send(HTU21D_ADDR, 0xE3);
	//odczytanie temperatury:
	_delay_ms(60);
	MSB = i2c_read(HTU21D_ADDR, 0);
	LSB = i2c_read(HTU21D_ADDR, 1);
	i2c_stop();
	sei();
	
	sensorTempValue= ((unsigned int)MSB << 8) | LSB;
	sensorTempValue &= 0xFFFC; 
	
	minTemperature = maxTemperature = -46.85 + 175.72 * (sensorTempValue / 65536.0);
	getTime(minTempTime);
	getTime(maxTempTime);
	
	
	//wysłanie komendy o odczytanie wilgotności:
	cli();
	i2c_start();
	i2c_send(HTU21D_ADDR, 0xE5);
	
	//odczytanie wilgotności
	_delay_ms(60);
	MSB = i2c_read(HTU21D_ADDR, 0);
	LSB = i2c_read(HTU21D_ADDR, 1);
	i2c_stop();
	
	sensorHumValue= ((unsigned int)MSB << 8) | LSB;
	sensorHumValue &= 0xFFFC;
	minHumidity = maxHumidity = -6.0 + 125.0 * (sensorHumValue / 65536.0);
	getTime(minHumTime);
	getTime(maxHumTime);
	
	writeToSDCard(minTemperature, maxTemperature, minHumidity, maxHumidity, minTempTime, maxTempTime, minHumTime, maxHumTime);
	
	sei();
	
	while (1)
	{	
		wdt_reset();			
		//wysłanie komendy odczytu temperatury
		cli();
		i2c_start();
		i2c_send(HTU21D_ADDR, 0xE3);									
		//odczytanie temperatury:		
		_delay_ms(60);		
		MSB = i2c_read(HTU21D_ADDR, 0);								
		LSB = i2c_read(HTU21D_ADDR, 1);
		i2c_stop();
		sei();
				
		sensorTempValue= ((unsigned int)MSB << 8) | LSB;
		sensorTempValue &= 0xFFFC; //Zero out the status bits but keep them in place		 
						
		temperature = -46.85 + 175.72 * (sensorTempValue / 65536.0);
		sprintf(tempString, "%.1f", temperature);
		if (temperature < minTemperature)
		{	
			minTemperature = temperature;
			getTime(minTempTime);
			writeToSDCard(minTemperature, maxTemperature, minHumidity, maxHumidity, minTempTime, maxTempTime, minHumTime, maxHumTime);			
		}
		else if (temperature > maxTemperature) 
		{
			maxTemperature = temperature;
			getTime(maxTempTime);
			writeToSDCard(minTemperature, maxTemperature, minHumidity, maxHumidity, minTempTime, maxTempTime, minHumTime, maxHumTime);
		}
		
		//wysłanie komendy o odczytanie wilgotności:
		cli();
		i2c_start();
		i2c_send(HTU21D_ADDR, 0xE5);			
		
		//odczytanie wilgotności
		_delay_ms(60);
		MSB = i2c_read(HTU21D_ADDR, 0);				
		LSB = i2c_read(HTU21D_ADDR, 1);
		i2c_stop();
		sei();
		sensorHumValue= ((unsigned int)MSB << 8) | LSB;
		sensorHumValue &= 0xFFFC; //Zero out the status bits but keep them in place
		humidity = -6.0 + 125.0 * (sensorHumValue / 65536.0);
		if (humidity < minHumidity)
		{
			minHumidity = humidity;
			getTime(minHumTime);
			writeToSDCard(minTemperature, maxTemperature, minHumidity, maxHumidity, minTempTime, maxTempTime, minHumTime, maxHumTime);
		}
		else if (humidity > maxHumidity)
		{
			maxHumidity = humidity;
			getTime(maxHumTime);
			writeToSDCard(minTemperature, maxTemperature, minHumidity, maxHumidity, minTempTime, maxTempTime, minHumTime, maxHumTime);
		}		
		sprintf(humString, "%.1f", humidity);
		
		cli();
		readTime(&hours, &minutes, &seconds, &day, &month, &year);							
		printTime(hours, minutes, seconds, day, month, year);
		sei();
		
		sendToLcd(0x94, instruction);
		sendTextToLcd("                ");
		sendToLcd(0x94, instruction);
		sendTextToLcd("Temp.: ");		
		sendTextToLcd(tempString);
		sendToLcd(0xDF, data);
		sendTextToLcd("C");
		
		sendToLcd(0xD4, instruction);
		sendTextToLcd("                ");
		sendToLcd(0xD4, instruction);		
		sendTextToLcd("Wilg.: ");
		sendTextToLcd(humString);
		sendTextToLcd("%");										
	}

}

/*
Funkcja ustawiająca datę po pierwszym włączeniu zasilania.
*/
void setStartDate() {

	//ustawienie dnia
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x05, 0x01);	
	i2c_stop();
	
	//ustawienie miesiaca
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x07, 0x01);
	i2c_stop();
	
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x08, toBCD((unsigned char)14));
	i2c_stop();
}

/*
Funkcja ustawiająca czas w zegarku RTC.
*/
void setTime(unsigned char hour, unsigned char minute, unsigned char second, unsigned char day, unsigned char month, unsigned char year)
{
	//ustawienie godziny:
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x04, hour);
	i2c_stop();
	
	//ustawienie minut:
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x03, minute);
	i2c_stop();
	
	//ustawienie sekund:
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x02, second);
	i2c_stop();
	
	//ustawienie dnia
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x05, day);
	i2c_stop();
	
	//ustawienie miesiąca
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x07, month);
	i2c_stop();
	
	//ustawienie roku:
	i2c_start();
	i2c_sendToSlaveReg(RTC_ADDR, 0x08, year);
	i2c_stop();
}

void getTime(char * timeString) 
{
	unsigned char hours;
	unsigned char minutes;
	unsigned char seconds;
	unsigned char day;
	unsigned char month;
	unsigned char year;
	
	readTime(&hours, &minutes, &seconds, &day, &month, &year);
	sprintf(timeString, "%02d.%02d.%02d %02d:%02d:%02d", day, month, year, hours, minutes, seconds);	
}

/*
Funkcja odczytująca czas z zegarka RTC
*/
void readTime(unsigned char * hours, unsigned char * minutes, unsigned char * seconds, unsigned char * day, unsigned char * month, unsigned char * year)
{	
	i2c_start();
	i2c_send(RTC_ADDR, 0x02);	
	*seconds = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x03);	
	*minutes = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x04);
	*hours = i2c_read(RTC_ADDR, 0);	
	i2c_stop();
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x05);
	*day = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x07);
	*month = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	
	i2c_start();
	i2c_send(RTC_ADDR, 0x08);
	*year = i2c_read(RTC_ADDR, 0);
	i2c_stop();
	
	*seconds = (((*seconds & 0x70) >> 4) % 10 * 10) + ((*seconds & 0x0F) % 10);
	*minutes = (((*minutes & 0x70) >> 4) % 10 * 10) + ((*minutes & 0x0F) % 10);
	*hours = (((*hours & 0x30) >> 4) % 10 * 10) + ((*hours & 0x0F) % 10);
	
	*day = (((*day & 0x30) >> 4) % 10 * 10) + ((*day & 0x0F) % 10);
	*month = (((*month & 0x10) >> 4) % 10 * 10) + ((*month & 0x0F) % 10);
	*year = (((*year >> 4) * 10) + (*year & 0x0F) % 10);	
}


void printTime(unsigned char hour, unsigned char minute, unsigned char second, unsigned char day, unsigned char month, unsigned char year)
{
	char timeString[10];
	char dateString[10];
	
	sprintf(timeString, "%02d:%02d:%02d", hour, minute, second);
	sendToLcd(0x80, instruction);
	sendTextToLcd(timeString);
	
	sprintf(dateString, "%02d.%02d.%02d", day, month, year);
	sendToLcd(0xC0, instruction);
	sendTextToLcd(dateString);
}

/*
Funkcja konwertujaca bajt do formatu BCD 8421
*/
unsigned char toBCD(unsigned char value)
{
	unsigned char bcd;
	bcd = (((value / 10) % 10) << 4) + (value % 10);
	return bcd;
}

ISR(INT0_vect)
{
	unsigned char seconds;
	unsigned char minutes;
	unsigned char hours;
	unsigned char day;
	unsigned char month;
	unsigned char year;
	
	cli();
	wdt_disable();
	_delay_ms(200);
	
	
	char selectedDateElement = HOUR;
	readTime(&hours, &minutes, &seconds, &day, &month, &year);
	clearLcd();
	printTime(hours, minutes, seconds, day, month, year);
			
	while(1)
	{
		sendToLcd(0x0F, instruction);
		if ((PIND & (1<<PORTD2)) == 0)
		{
			_delay_ms(100);
			if ((PIND & (1<<PORTD2)) == 0)
			{
				selectedDateElement++;
				_delay_ms(100);
			}
		}
		
		if ((PINC & (1<<PORTC7)) == 0)
		{
			_delay_ms(100);
			if ((PINC & (1<<PORTC7)) == 0)
			{
				switch(selectedDateElement)
				{
					case HOUR:
					hours++;
					if (hours == 24)
					hours = 0;
					break;
					case MINUTE:
					minutes++;
					if (minutes == 60)
					minutes = 0;
					break;
					case SECOND:
					seconds++;
					if (seconds == 60)
					seconds = 0;
					break;
					case DAY:
					day++;
					if (day == 32)
					day = 1;
					break;
					case MONTH:
					month++;
					if (month == 13)
					month = 1;
					break;
					case YEAR:
					year++;
					if (year == 100)
					year = 0;
					break;
				}
			}
			sendToLcd(0x0c, instruction);	//wlacz wyswietlacz bez kursora
			printTime(hours, minutes, seconds, day, month, year);
			_delay_ms(100);
		}
		
		if ((PINC & (1<<PORTC6)) == 0)
		{
			_delay_ms(100);
			if ((PINC & (1<<PORTC6)) == 0)
			{
				switch(selectedDateElement)
				{
					case HOUR:
					if (hours == 0)
					hours = 23;
					else
					hours--;
					break;
					case MINUTE:
					if (minutes == 0)
					minutes = 59;
					else
					minutes--;
					break;
					case SECOND:
					if (seconds == 0)
					seconds = 59;
					else
					seconds--;
					break;
					case DAY:
					if (day == 1)
					day = 31;
					else
					day--;
					break;
					case MONTH:
					if (month == 1)
					month = 12;
					else
					month--;
					break;
					case YEAR:
					if (year == 0)
					year = 99;
					else
					year--;
					break;
				}
			}
			sendToLcd(0x0c, instruction);	//wlacz wyswietlacz bez kursora
			printTime(hours, minutes, seconds, day, month, year);
			_delay_ms(100);
		}
		
		switch(selectedDateElement)
		{
			case HOUR:
			sendToLcd(0x80, instruction);
			break;
			case MINUTE:
			sendToLcd(0x83, instruction);
			break;
			case SECOND:
			sendToLcd(0x86, instruction);
			break;
			case DAY:
			sendToLcd(0xC0, instruction);
			break;
			case MONTH:
			sendToLcd(0xC3, instruction);
			break;
			case YEAR:
			sendToLcd(0xC6, instruction);
			break;
			default:
			sendToLcd(0x0c, instruction);	//wlacz wyswietlacz bez kursora
			setTime(toBCD(hours), toBCD(minutes), toBCD(seconds), toBCD(day), toBCD(month), toBCD(year));
			//GICR = 1<<INT0;	 // Enable INT0
			_delay_ms(50);
			sei();
			wdt_enable(WDTO_1S);
			return;
		}
	}
}


void writeToSDCard(double minTemperature, double maxTemperature, double minHumidity, double maxHumidity, char * minTempTime, char * maxTempTime, char * minHumTime, char * maxHumTime)
{
	FATFS FatFs;
	char dataString[255];
	UINT bw;
	
	cli();
	if (f_mount(&FatFs, "", 0) == FR_OK) 
	{
		sprintf(dataString, "%s\t%.1f st. C\ttemp. minimalna \r\n%s\t%.1f st. C\ttemp. maksymalna \r\n%s\t%.1f %%    \twilg. minimalna \r\n%s\t%.1f %%    \twilg. maksymalna", minTempTime, minTemperature, maxTempTime, maxTemperature, minHumTime, minHumidity, maxHumTime, maxHumidity);
		if (f_open(&Fil, "log.txt", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
			f_write(&Fil, dataString, strlen(dataString), &bw);
			f_close(&Fil);							
			setFileTime("log.txt");
		}
		f_mount(NULL, "", 0);
	}
	sei();
}

void setFileTime(char * fileName)
{
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned char day;
	unsigned char month;
	unsigned char year;
	readTime(&hour, &minute, &second, &day, &month, &year);
	FILINFO fno;

	fno.fdate = (WORD)(((2000 + year - 1980) * 512U) | month * 32U | day);
	fno.ftime = (WORD)(hour * 2048U | minute * 32U | second / 2U);
	f_utime(fileName, &fno);
}