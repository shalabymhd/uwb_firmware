import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
"""
This script publishes a message to the USB port continuously until terminated.
"""
ports = find_uwb_serial_ports()
uwb1 = UwbModule("/dev/ttyACM0", verbose=True)


data = {
    "t": 3.14159,
    "x":[1,2,3],
    "P":[[1,0,0],[0,1,0],[0,0,1]],
}
uwb1.broadcast(data)
time.sleep(1)
