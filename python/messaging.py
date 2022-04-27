import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
import msgpack
"""
This script publishes a message to the USB port continuously until terminated.
"""
long_msg = True
ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[0], timeout = 1, verbose=True)
uwb2 = UwbModule(ports[1],  timeout = 1, verbose=True)

print(uwb1.get_id())
print(uwb2.get_id())

def msg_callback(msg, is_valid):
    try:
        uwb2.output(msgpack.unpackb(msg))
    except:
        pass

uwb2.register_message_callback(msg_callback)

if long_msg:
    test_msg = {
    "t": 3.14159,
    "x":[1.0]*15,
    "P":[[1.0]*i for i in range(1,15+1)],
    }
else:
    test_msg = {
    "t": 3.14159,
    "x":[1,2,3],
    "P":[[1,0,0],[0,1,0],[0,0,1]],
    }

counter = 1
while True:
    data = msgpack.packb(test_msg)
    uwb1.broadcast(data)

    print(counter)
    counter += 1

    # time.sleep(0.01)