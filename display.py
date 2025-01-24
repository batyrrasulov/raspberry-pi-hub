#!/usr/bin/python3

import time
import mysql.connector
import drivers
import socket
from datetime import datetime

SERVER_ADDRESS = '127.0.0.1'
SERVER_PORT = 8080
DATABASE_NAME = 'strawberrypi'

display = drivers.Lcd()

# Set up server socket start listening for the C files
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((SERVER_ADDRESS, SERVER_PORT))
server_socket.listen(1)
print("Waiting for C code...")

# Accept the connect with the C files
client_socket, client_address = server_socket.accept()
print(f"Connection established with {client_address}")

try:
    db_conn = mysql.connector.connect(
        host="localhost",
        user="root",
        password="root",
        database=DATABASE_NAME
    )
    print("Connected to DB successfully...")
    cursor = db_conn.cursor()
except mysql.connector.Error as err:
    print(f"Error: {err}")

# Function to separate the data received from main.c
def parse_data(data):
    try:
        if data == "ERROR":
            return None, None, None
        
        data_parts = data.split()

        if len(data_parts) == 2:
            # If there are 2 data points, treat them as CO and LPG levels
            co_ppm, lpg_ppm = data_parts
            co_ppm = float(co_ppm)
            lpg_ppm = float(lpg_ppm)
            return co_ppm, lpg_ppm, 0  # Return 0 for the third value as flag

        elif len(data_parts) == 3:
            # If there are 3 data points, parse as temperature, humidity, and light levels
            temp_str, humi_str, light_str = data_parts
            temperature = float(temp_str)
            humidity = float(humi_str.replace('%', ''))
            light_levels = int(light_str)
            return temperature, humidity, light_levels

        else:
            # If data doesn't match expected formats, return None
            return None, None, None

    except Exception as e:
        print("Error parsing data:", data)
        return None, None, None


# Function to retrieve rolling 7 day trends
def fetch_weekly_trends():
    query = """
    SELECT 
        AVG(temperature) AS avg_temp,
        AVG(humidity) AS avg_humidity,
        AVG(light) AS avg_light
    FROM SensorData
    WHERE timestamp >= NOW() - INTERVAL 7 DAY
    """
    
    cursor.execute(query)
    avg_temp, avg_humidity, avg_light = cursor.fetchone()  # Fetch the result of the query

    display_data = (
        f"Avg Temp: {avg_temp:.1f} C        "
        f"Avg Humidity: {avg_humidity:.1f}%        "
        f"Avg Light: {avg_light}                "
    )

    display.lcd_clear()
    display.scroll_text(f"{display_data}", 0.5)            

try:
    print("Waiting to receive data...")

    display.lcd_display_string(f"Welcome!", 1)
    time.sleep(5)
    display.lcd_display_string(f"Getting things", 1)
    display.lcd_display_string(f"ready for you!", 2)
    time.sleep(5)
    display.lcd_clear()

    while True:
        data = client_socket.recv(1024).decode('utf-8')
        if data:
            display.lcd_backlight(1)

            temperature, humidity, light_level = parse_data(data)  # Parse the data
            
            if temperature is not None and humidity is not None and light_level != 0:
                temperature_F = (temperature * 9/5) + 32

                current_time = datetime.now().strftime("%B %d, %Y %I:%M %p") 

                print(f"Time: {current_time}")
                print(f"Temp: {temperature:.1f} C")
                print(f"Humidity: {humidity:.1f} %")
                print(f"Temp: {temperature_F:.1f} F")
                print(f"Light Level: {light_level}")


                # Display the date and time
                display.scroll_text(f"{current_time}                ", 0.5)
                display.lcd_clear()

                # Display the temperature in C and humidity in the first 10 seconds
                display.lcd_display_string(f"Temp: {temperature:.1f} C", 1)
                display.lcd_display_string(f"Humidity: {humidity:.1f}%", 2)
                time.sleep(10)
                display.lcd_clear()

                # Display the temperature in F and light levels in the next 10 seconds
                display.lcd_display_string(f"Temp: {temperature_F:.1f} F", 1)
                display.lcd_display_string(f"Light Level: {light_level}", 2)
                time.sleep(10)
                display.lcd_clear()

                # Display the rolling 7 day trends
                fetch_weekly_trends()
                display.lcd_clear()
                display.lcd_backlight(0)

            # If we receive the flag that there is a gas detected, we will display that
            elif temperature is not None and humidity is not None and light_level == 0:
                display.lcd_display_string(f"Gas detected", 1)
                time.sleep(10)
                display.lcd_clear()
                display.lcd_backlight(0)
                continue

            else:
                display.scroll_text("Error reading from sensors...        ", 0.5)
                time.sleep(1)
                display.lcd_clear()
                display.scroll_text("Will try again momentarily...                ", 0.5)
                time.sleep(1)
                display.lcd_clear()
                display.lcd_backlight(0)

except KeyboardInterrupt:
    print("Keyboard Interrupt detected...")
except Exception as e:
    print(f"Error: {e}")

finally:
    print("Cleaning up...")
    display.lcd_clear() 
    client_socket.close() 
    server_socket.close()
    cursor.close()
    db_conn.close()
    
