#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <time.h>

#include "dhthw.h"

int dht11_data[5] = {0, 0, 0, 0, 0};

/**
 * This function initalizes the sensor according to DHT11 Humidity &
 * Temperature Sensor Data Sheet (Section 5.2)
 * https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
 */
void initializeDHT11()
{
    pinMode(DHT11_PIN, OUTPUT);

    // Send a low status for 18 microseconds
    digitalWrite(DHT11_PIN, LOW);
    delay(18);

    // Send a high status for 40 microseconds
    digitalWrite(DHT11_PIN, HIGH);
    delayMicroseconds(40);

    // Get it ready to receive input
    pinMode(DHT11_PIN, INPUT);
}

/**
 * This function just determines if a pulse is a 0 or 1 based on the pulse timing.
 * It stores the value and moves onto the next bit.
 */
void parseData(uint8_t *bit, uint8_t microseconds)
{
    // Shift left to make space for the new bit
    dht11_data[*bit / 8] <<= 1;

    // If the pulse duration is more than 16 microseconds, it's a 1
    if (microseconds > 16)
    {
        dht11_data[*bit / 8] |= 1;
    }

    // Move on to the next bit
    (*bit)++;
}

/**
 * This function makes makes sure the data is valid according to the
 * DHT11 Humidity & Temperature Sensor Data Sheet, last byte should be the sum
 * of the previous four
 */
int checkChecksum()
{
    return (dht11_data[4] == ((dht11_data[0] + dht11_data[1] + dht11_data[2] + dht11_data[3]) & 0xFF));
}

/**
 * This function reads the temperature and humidity from the DHT11.
 */
int *readDHT11()
{
    uint8_t laststate = HIGH;
    uint8_t microseconds = 0; // Will use this to time the pulse
    uint8_t bit = 0, i;
    float f;

    // Erase any previously stored data for a new reading
    dht11_data[0] = dht11_data[1] = dht11_data[2] = dht11_data[3] = dht11_data[4] = 0;

    initializeDHT11();

    for (i = 0; i < ITERATIONS; i++)
    {

        microseconds = 0;

        // Wait for the signal received to change state
        while (digitalRead(DHT11_PIN) == laststate)
        {
            microseconds++;
            delayMicroseconds(1);
            if (microseconds == 255)
                break;
        }
        laststate = digitalRead(DHT11_PIN);

        if (microseconds == 255)
            break;

        // Skip first four iterations and make sure we're on an even iteration
        if ((i >= 4) && (i % 2 == 0))
        {
            // Read each bit individually
            parseData(&bit, microseconds);
        }
    }

    // If we finally read all 40 bits and the checksum is good then return the data
    if ((bit >= 40) && checkChecksum())
    {
        f = (dht11_data[2] * (float)(9 / 5)) + 32;
        return dht11_data;
    }
    else
    {
        printf("Error reading from DHT11\n");
        return NULL;
    }
}