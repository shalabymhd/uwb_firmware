import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
import json
"""
This script publishes a message to the USB port continuously until terminated.
"""
ports = find_uwb_serial_ports()
uwb1 = UwbModule("/dev/ttyACM0", verbose=False)
uwb2 = UwbModule("/dev/ttyACM1", verbose=False)

def rx_callback(msg):
    x = json.loads(msg)
    print(x)

uwb2.register_callback("R06",rx_callback)


data = {
    "t": 3.14159,
    "x":[1,2,3],
    "P":[[1,0,0],[0,1,0],[0,0,1]],
}
counter = 0
while True:
    uwb1.broadcast(data)
    time.sleep(0.001)
    counter += 1
    print(counter)
