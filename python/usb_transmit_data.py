
import serial
import time
from pyuwb import UwbModule
import pypozyx
"""
This script publishes a message to the USB port continuously until terminated.
"""

#ser1 = serial.Serial('/dev/ttyACM0', 19200, timeout=1)\
# ser3 = serial.Serial('/dev/ttyACM3', 19200, timeout=1)
uwb = UwbModule("/dev/ttyACM0", verbose=False)

while True:
    # dataToTransmit = "C02,6,0\r"
    # ser1.write(dataToTransmit.encode())
    # print(ser1.read(ser1.in_waiting).decode(), end="")

    #data = uwb.get_id()
    data = uwb.do_twr(3, meas_at_target=False)
    print(data) 
    time.sleep(0.01)
    # dataToTransmit = "C02,6,0\r"
    # ser3.write(dataToTransmit.encode())
    # time.sleep(0.01)

ser1.close()
