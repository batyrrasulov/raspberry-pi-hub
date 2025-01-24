#include "adchw.h"

/**
 * Function to read from the ADS7830 ADC
 */
int read_adc(int *fd, int channel)
{
    // Just to make sure the channel used is between 0 and 8
    if (channel < 0 || channel > 7)
    {
        fprintf(stderr, "Invalid channel number!\n");
        return -1;
    }

    // 0x84 is the start command for the ADS7830
    // We add the channel number and shift it by four bits to tell
    // the ADC which channel to read
    int command = 0x84 | (channel << 4);
    wiringPiI2CWrite(*fd, command);

    // Sleep for 10 ms to let ADC process command
    usleep(10000);

    // Read and return the value from the ADC
    int value = wiringPiI2CRead(*fd);
    return value;
}

/**
 * Initialize the ADC with the I2C function from Wiring Pi and
 * return the file descriptor
 */
int *initializeADC()
{
    // Allocate memory for the file descriptor
    int *fd = (int *)malloc(sizeof(int));

    *fd = wiringPiI2CSetup(ADS7830_ADDR);

    // If the file descriptor is -1, then the I2C setup failed
    if (*fd == -1)
    {
        fprintf(stderr, "I2C setup failed\n");
        return NULL;
    }
    return fd;
}