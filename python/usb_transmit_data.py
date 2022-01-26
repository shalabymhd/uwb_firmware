#!/usr/bin/env python2
import serial
import time
"""
This script publishes a message to the USB port continuously until terminated.
"""

ser1 = serial.Serial('/dev/ttyACM1', 19200, timeout=1)
ser2 = serial.Serial('/dev/ttyACM2', 19200, timeout=1)
# ser3 = serial.Serial('/dev/ttyACM3', 19200, timeout=1)

while True:
    # dataToTransmit = "Hello World from the python script.\n"
    # dataToTransmit = "Im python.\r"
    dataToTransmit = "C02,6,0\r"
    ser1.write(dataToTransmit.encode())
    # print(ser1.readline().decode())
    time.sleep(0.01)
    dataToTransmit = "C02,3,0\r"
    ser2.write(dataToTransmit.encode())
    time.sleep(0.01)
    # ser3.write(dataToTransmit.encode())
    # time.sleep(0.01)

ser1.close()
