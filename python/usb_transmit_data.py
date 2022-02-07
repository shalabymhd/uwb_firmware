
import serial
import time
from pyuwb import UwbModule
import pypozyx
"""
This script publishes a message to the USB port continuously until terminated.
"""

#ser1 = serial.Serial('/dev/ttyACM0', 19200, timeout=1)\
# ser3 = serial.Serial('/dev/ttyACM3', 19200, timeout=1)
uwb1 = UwbModule("/dev/ttyACM1")
uwb2 = UwbModule("/dev/ttyACM2")

data1 = uwb1.set_idle()
print(data1)

data1 = uwb1.toggle_passive(True)
print(data1)

while True:
    # dataToTransmit = "C02,6,0\r"
    # ser1.write(dataToTransmit.encode())
    # print(ser1.read(ser1.in_waiting).decode(), end="")

    data1 = uwb1.do_twr(2,True)
    #data = uwb.do_twr(6, meas_at_target=False)
    print(data1) 
    time.sleep(0.01)

    data2 = uwb2.do_twr(2,True)
    #data = uwb.do_twr(6, meas_at_target=False)
    # print(data1) 
    time.sleep(0.01)
    # dataToTransmit = "C02,6,0\r"
    # ser3.write(dataToTransmit.encode())
    # time.sleep(0.01)

ser1.close()
