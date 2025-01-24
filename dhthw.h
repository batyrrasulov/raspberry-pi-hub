#ifndef DHTHW_H
#define DHTHW_H

#include <stdint.h>

#define DHT11_PIN 21
#define ITERATIONS 85

extern int dht11_data[5];

void initializeDHT11();
void parseData(uint8_t *bit, uint8_t microseconds);
int checkChecksum();
int *readDHT11();

#endif // DHTHW_H
