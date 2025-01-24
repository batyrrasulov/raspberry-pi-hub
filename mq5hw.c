#include "mq5hw.h"

/**
 * Function to compare the ratio according to the graph in the
 * Gas Sensor (MQ5) User Manual (Section 5.3), in order to return carbon
 * monoxide levels in PPM.
 * (I'm aware the graphs are not linear however to make data mapping easier,
 * I made a linear representation for each interval between points)
 * https://www.mouser.com/datasheet/2/744/Seeed_101020056-1217478.pdf?srsltid=AfmBOoqBuHrLJZkh3vY91wNPHuAFAUB7isGpq2N8C9ZTG7UltKnCZD1e
 */
float ratioToCOPPM(float ratio)
{
    float CO_ppm = 0.0;
    if (ratio >= 3.25 && ratio <= 3.9)
    {
        CO_ppm = (ratio - 4.334) / (-0.00217); // y = -0.00217x + 4.334
    }
    else if (ratio >= 3.1 && ratio < 3.25)
    {
        CO_ppm = (ratio - 3.5) / (-0.0005); // y = -0.0005x + 3.5
    }
    else if (ratio >= 2.9 && ratio < 3.1)
    {
        CO_ppm = (ratio - 3.9) / (-0.001); // y = -0.001x + 3.9
    }
    else if (ratio >= 2.8 && ratio < 2.9)
    {
        CO_ppm = (ratio - 3.07) / (-0.00017); // y = -0.00017x + 3.07
    }
    else if (ratio >= 2.75 && ratio < 2.8)
    {
        CO_ppm = (ratio - 3.01) / (-0.00013); // y = -0.00013x + 3.01
    }
    else if (ratio >= 2.5 && ratio < 2.75)
    {
        CO_ppm = (ratio - 3.25) / (-0.00025); // y = -0.00025x + 3.25
    }
    else if (ratio >= 2.4 && ratio < 2.5)
    {
        CO_ppm = (ratio - 2.65) / (-0.00005); // y = -0.00005x + 2.65
    }
    else if (ratio >= 2.25 && ratio < 2.4)
    {
        CO_ppm = (ratio - 2.55) / (-0.00003); // y = -0.00003x + 2.55
    }
    return CO_ppm;
}

/**
 * Function to compare the ratio according to the graph in the
 * Gas Sensor (MQ5) User Manual (Section 5.3), in order to return liquified
 * petroleum gas (LPG) levels in PPM.
 * (I'm aware the graphs are not linear however to make data mapping easier,
 * I made a linear representation for each interval between points)
 * https://www.mouser.com/datasheet/2/744/Seeed_101020056-1217478.pdf?srsltid=AfmBOoqBuHrLJZkh3vY91wNPHuAFAUB7isGpq2N8C9ZTG7UltKnCZD1e
 */
float ratioToLPGPPM(float ratio)
{
    float LPG_ppm = 0.0;
    if (ratio >= 0.49 && ratio <= 0.7)
    {
        LPG_ppm = (ratio - 0.84) / (-0.0007); // y = -0.0007x + 0.84
    }
    else if (ratio >= 0.4 && ratio < 0.49)
    {
        LPG_ppm = (ratio - 0.64) / (-0.0003); // y = -0.0003x + 0.64
    }
    else if (ratio >= 0.37 && ratio < 0.4)
    {
        LPG_ppm = (ratio - 0.52) / (-0.00015); // y = -0.00015x + 0.52
    }
    else if (ratio >= 0.3 && ratio < 0.37)
    {
        LPG_ppm = (ratio - 0.492) / (-0.00012); // y = -0.00012x + 0.492
    }
    else if (ratio >= 0.28 && ratio < 0.3)
    {
        LPG_ppm = (ratio - 0.38) / (-0.00005); // y = -0.00005x + 0.38
    }
    else if (ratio >= 0.24 && ratio < 0.28)
    {
        LPG_ppm = (ratio - 0.36) / (-0.00004); // y = -0.00004x + 0.36
    }
    else if (ratio >= 0.19 && ratio < 0.24)
    {
        LPG_ppm = (ratio - 0.33) / (-0.00003); // y = -0.00003x + 0.33
    }
    else if (ratio >= 0.05 && ratio < 0.19)
    {
        LPG_ppm = (ratio - 0.25) / (-0.00001); // y = -0.00001x + 0.25
    }
    return LPG_ppm;
}

/**
 * Function to calibrate the MQ5 according to according to
 * Gas Sensor (MQ5) User Manual (Section 5.3)
 * https://www.mouser.com/datasheet/2/744/Seeed_101020056-1217478.pdf?srsltid=AfmBOoqBuHrLJZkh3vY91wNPHuAFAUB7isGpq2N8C9ZTG7UltKnCZD1e
 */
float calibrateMQ5(int *fd)
{
    printf("Warming up the MQ-5\n");
    sleep(10);

    float averageSensorValue = 0;

    for (int i = 0; i < 1000; i++)
    {
        averageSensorValue += read_adc(fd, MQ5_CHANNEL); // Read raw sensor value
    }
    averageSensorValue /= 1000.0; // Calculate average

    float voltage = (averageSensorValue / 1024) * 5.0; // Convert reading to voltage
    float rsAir = (5.0 - voltage) / voltage;           // Calculate resistance in clear air
    float r0 = rsAir / 6.5;                            // Calculate R0 based on known ratio in clear air

    printf("Voltage: %.2f\nResistance in clear air: %.2f\nR0 (Resistance in Clean Air): %.2f\n", voltage, rsAir, r0);

    return r0;
}

/**
 * Function to read the ratio (which will be used to calculate
 * the CO and LPG levels) from the MQ-5 according to
 * Gas Sensor (MQ5) User Manual (Section 5.3)
 * https://www.mouser.com/datasheet/2/744/Seeed_101020056-1217478.pdf?srsltid=AfmBOoqBuHrLJZkh3vY91wNPHuAFAUB7isGpq2N8C9ZTG7UltKnCZD1e
 */
float *readMQ5(int *fd, float *r0)
{
    // This is where we're going to store and send back the values for
    // carbon monoxide and LPG in PPM
    float *values = (float *)malloc(2 * sizeof(float));

    if (values == NULL)
    {
        printf("Memory allocation failed\n");
        return NULL; // Return NULL if malloc fails
    }

    float sensor_volt;
    float RS_gas; // Get value of RS in a GAS
    float ratio;  // Get ratio RS_GAS/RS_air
    int sensorValue = read_adc(fd, MQ5_CHANNEL);
    sensor_volt = (float)sensorValue / 1024 * 5.0;
    RS_gas = (5.0 - sensor_volt) / sensor_volt;
    ratio = RS_gas / *r0; // ratio = RS/R0

    printf("Ratio: %.2f\n", ratio);

    *values = ratioToCOPPM(ratio);        // Carbon monoxide in PPM
    *(values + 1) = ratioToLPGPPM(ratio); // LPG in PPM

    return values;
}