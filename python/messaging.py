import time
from pyuwb import UwbModule, find_uwb_serial_ports
"""
This script publishes a message to the USB port continuously until terminated.
"""
ports = find_uwb_serial_ports()
print(ports)
uwb1 = UwbModule("/dev/ttyACM0", verbose=True)
uwb2 = UwbModule("/dev/ttyACM1", verbose=True)
id1 = uwb1.get_id()
id2 = uwb2.get_id()
print(id1)
print(id2)
uwb1.broadcast()
time.sleep(1)
