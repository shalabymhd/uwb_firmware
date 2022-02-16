
import serial
import time
from pyuwb import UwbModule
import pypozyx
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM1", verbose=False)
# uwb2 = UwbModule("/dev/ttyACM2", verbose=False)

uwb1.toggle_passive(toggle=True)

# Passive-listening callback.
def cb_timestamps(ts1,ts2,ts3,ts4):
    dict = {"tx1":ts1, "rx1":ts2, "tx2":ts3, "rx2":ts4}
    print(dict)

uwb1.register_callback("R99",cb_timestamps)

while True:
    data1 = uwb1.do_twr(2,mult_twr=1)
    print(data1) 
    time.sleep(0.003)

    # data2 = uwb2.do_twr(3,meas_at_target=True)
    # print(data1) 
    # time.sleep(0.005)

ser1.close()
