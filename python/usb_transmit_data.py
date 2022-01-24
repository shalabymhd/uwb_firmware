#!/usr/bin/env python2
import serial
import time
"""
This script publishes a message to the USB port continuously until terminated.
"""

ser1 = serial.Serial('/dev/ttyACM1', 19200, timeout=1)
ser2 = serial.Serial('/dev/ttyACM2', 19200, timeout=1)
ser3 = serial.Serial('/dev/ttyACM3', 19200, timeout=1)

while 1:
    # dataToTransmit = "Hello World from the python script.\n"
    # dataToTransmit = "Im python.\r"
    dataToTransmit = "C02,8,CATS,C1480000,1,0,4386B000\r"
    ser1.write(dataToTransmit.encode())
    time.sleep(0.01)
    ser2.write(dataToTransmit.encode())
    time.sleep(0.01)
    ser3.write(dataToTransmit.encode())
    time.sleep(0.01)