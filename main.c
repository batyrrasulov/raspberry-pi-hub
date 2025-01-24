#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiringPi.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mariadb/mysql.h>

#include "dhthw.h"
#include "adchw.h"
#include "mq5hw.h"

#define PHOTORESISTOR_CHANNEL 0

#define SERVER_PORT 8080
#define SQL_PORT 3306
#define DATABASE_NAME "strawberrypi"
#define DATABASE_USERNAME "root"
#define DATABASE_PASSWORD "root"

int sockfd;
struct sockaddr_in server;

/**
 * Function to set up the socket connection
 */
void setupSocketConnection()
{
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // If socket creation fails, return error
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    // Define server address

    // We're going to use IPv4
    server.sin_family = AF_INET;
    // Set server port
    server.sin_port = htons(SERVER_PORT);
    // Set server IP to localhost
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connection to server failed");
        exit(1);
    }

    printf("Connected to the Python server.\n");
}

/**
 * Function to "rest", however we will still read gas levels every five seconds.
 * If there is a gas detection, we will send the data to the database and
 * we will make it so the frontend displays the detection
 */
void rest(int *fd, float *r0, int min, MYSQL *con)
{
    int seconds = 0;
    int maxSeconds = min * 60;
    while (seconds < maxSeconds)
    {
        float *MQ5Values = readMQ5(fd, r0);
        float co_ppm = MQ5Values[0];
        float lpg_ppm = MQ5Values[1];

        if (co_ppm > 200 || lpg_ppm > 200)
        {
            // Construct SQL query to update GasData Table
            char query[256];
            snprintf(query, sizeof(query),
                     "INSERT INTO GasData (data_id, co_ppm, lpg_ppm) VALUES (NOW(), %.2f, %.2f)",
                     co_ppm, lpg_ppm);

            if (mysql_query(con, query))
            {
                printf("Error inserting into GasData\n");
            }

            // Prepare the temp to send to the Python server
            char data[40];
            snprintf(data, sizeof(data), "%.2f %.2f", co_ppm, lpg_ppm);

            // Send the temp to the server
            send(sockfd, data, strlen(data), 0);

            sleep(10);
            seconds += 10;
        }

        free(MQ5Values);

        // Wait 3 seconds between each read
        sleep(3);
        seconds += 3;
    }
}

int main(void)
{
    // Initialize WiringPi
    if (wiringPiSetupGpio() == -1)
    {
        printf("Failed to initialize WiringPi.\n");
        return 1;
    }

    setupSocketConnection(); // Set up the socket server

    MYSQL *con = mysql_init(NULL);

    if (!con)
    {
        fprintf(stderr, "MYSQL connection failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, "localhost", DATABASE_USERNAME, DATABASE_PASSWORD,
                           DATABASE_NAME, SQL_PORT, NULL, 0) == NULL)
    {
        printf("Error connecting to DB\n");
    }

    // Open the I2C device (ADS7830)
    int *fd = initializeADC();

    float r0 = calibrateMQ5(fd);

    while (1)
    {
        int *dht11_data = readDHT11(); // Get data from the DHT-11

        if (dht11_data)
        {

            float temperature = dht11_data[2] + dht11_data[3] / 10.0;
            float humidity = dht11_data[0] + dht11_data[1] / 10.0;
            int light_level = read_adc(fd, PHOTORESISTOR_CHANNEL);

            printf("Data Sent:\nTemp: %.1f C\nHumidity: %.1f%%\nLight Level: %d\n",
                   temperature, humidity, light_level);

            // Write the SQL query to upload sensor data
            char query[256];
            snprintf(query, sizeof(query),
                     "INSERT INTO SensorData (timestamp, temperature, humidity, light) VALUES (NOW(), %.1f, %.1f, %d)",
                     temperature, humidity, light_level);

            // Try to execute the query
            if (mysql_query(con, query))
            {
                printf("Error inserting into SensorData\n");
            }

            // Prepare to send to the Python server
            char data[40];
            snprintf(data, sizeof(data), "%.1f %.1f%% %d", temperature, humidity, light_level);

            // Send  to the server
            send(sockfd, data, strlen(data), 0);
        }
        else
        {
            // Create an error message
            char message[20];
            snprintf(message, sizeof(message), "ERROR");

            // Send the temp to the server
            send(sockfd, message, strlen(message), 0);
        };

        // Make it rest for 10 minutes but still read MQ-5 every 5 seconds
        rest(fd, &r0, 5, con);
    }

    return 0;
}
