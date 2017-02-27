#ifndef I2C_H_
#define I2C_H_

#include <compat/twi.h>

#define I2C_START 0
#define I2C_DATA  1
#define I2C_STOP  2
#define MAX_TRIES 50

void i2c_start(void);
void i2c_stop(void);
unsigned char i2c_send(unsigned char slaveAdress,  unsigned char data);
void i2c_sendToSlaveReg(unsigned char slaveAdress, unsigned char slaverRegisterAdress, unsigned char data);
unsigned char i2c_read(unsigned char slaveAdress, int secondByte);

#endif /* I2C_H_ */