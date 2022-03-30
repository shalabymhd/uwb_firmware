import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
import json
"""
This script publishes a message to the USB port continuously until terminated.
"""
ports = find_uwb_serial_ports()
uwb1 = UwbModule("/dev/ttyACM0", verbose=True)
uwb2 = UwbModule("/dev/ttyACM1", verbose=True)

def rx_callback(msg):
    x = json.loads(msg)
    pass

uwb2.register_callback("R06",rx_callback)


data = {
    "t": 3.14159,
    "x":[1,2,3],
    "P":[[1,0,0],[0,1,0],[0,0,1]],
}
uwb1.broadcast(data)


time.sleep(100)
