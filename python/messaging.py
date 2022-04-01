import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
import json
"""
This script publishes a message to the USB port continuously until terminated.
"""
uwb1 = UwbModule("/dev/ttyACM0", verbose=True)

id = uwb1.get_id()

