import time
from pyuwb import UwbModule, find_uwb_serial_ports
"""
This script publishes a message to the USB port continuously until terminated.
"""
ports = find_uwb_serial_ports()
print(ports)
uwb1 = UwbModule("/dev/ttyACM0", verbose=True)
my_id = uwb1.get_id()
print(my_id)
data = uwb1.do_twr(5)
