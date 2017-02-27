#include "i2c.h"

/*
Funkcja wysyłająca sygnał START na magistralę I2C.
*/
void i2c_start(void)
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); //wysłanie sygnału start
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
}

/*
Funkcja wysyłająca sygnał STOP na magistralę I2C.
*/
void i2c_stop(void)
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
	while ((TWCR & (1<<TWSTO)));
}

/*
Funkcja wysyłająca bajt do wybranego urządzenia podłączonego do magistrali I2C.
Parametry:
	slaveAddress - adres urządzenia
	data - bajt do wysłania
*/
unsigned char i2c_send(unsigned char slaveAdress,  unsigned char data)
{
	TWDR = slaveAdress | TW_WRITE;
	TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
	
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
	
	return 1;
}

/*
Funkcja wysyłająca bajt do wybranego rejestru urządzenia podłączonego do magistrali I2C.
Parametry:
	slaveAddress - adres urządzenia
	slaveRegisterAddress - adres rejestru
	data - bajt do wysłania
*/
void i2c_sendToSlaveReg(unsigned char slaveAdress, unsigned char slaverRegisterAdress, unsigned char data)
{
	TWDR = slaveAdress |TW_WRITE;
	TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
	
	TWDR = slaverRegisterAdress;
	TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
	
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
	while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
}

/*
Funkcja odczytująca dane z urządzenia podłączonego do magistrali I2C.
Parametry:
	slaveAddress - adres urządzenia
	isNotFirstByte - określa czy transmisja dotyczy kolejnego bajtu odebanego z urządzenia.
		0 - pierwszy bajt 
		1 - kolejne bajty
Wartość zwracana:
	bajt odebrany z urządzenia.
*/
unsigned char i2c_read(unsigned char slaveAdress, int isNotFirstByte)
{			
	if (!isNotFirstByte)
	{
		TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send (rep.) start condition */
		while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
		TWDR = slaveAdress | TW_READ;
		TWCR = (1<<TWINT) | (1<<TWEN); //sygnał do rozpoczęcia transmisji
		while (!(TWCR & (1<<TWINT))); //oczekiwanie na transmisję
	}
	
	TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
	while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */	
	
	return TWDR;
}

/*
unsignd char i2c_readFromSlaveReg(unsigned char slaveAdress, unsigned char slaveRegisterAdress)
{
	
}
*/

