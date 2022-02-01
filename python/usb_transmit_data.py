
import serial
import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

#ser1 = serial.Serial('/dev/ttyACM0', 19200, timeout=1)\
# ser3 = serial.Serial('/dev/ttyACM3', 19200, timeout=1)
uwb = UwbModule("/dev/ttyACM0")

while True:
    # dataToTransmit = "Hello World from the python script.\n"
    # # dataToTransmit = "Im python.\r"
    # dataToTransmit = "C02,5,1\r"
    # ser1.write(dataToTransmit.encode())
    # c = ""
    # resp = ""
    # while c != "\r":
    #     c = ser1.read(1).decode()
    #     resp += c
    # print(resp)
    data = uwb.do_twr(5, meas_at_target=False)
    print(data) 
    time.sleep(0.01)
    # dataToTransmit = "C02,6,0\r"
    # ser3.write(dataToTransmit.encode())
    # time.sleep(0.01)

ser1.close()
