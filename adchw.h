#ifndef ADCHW_H
#define ADCHW_H

#include <stdio.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <stdlib.h>

// Make sure this address is right
// You can check this by typing
// i2cdetect -y 1
// in your terminal
#define ADS7830_ADDR 0x4b

int read_adc(int *fd, int channel);
int *initializeADC();

#endif // ADCHW_H