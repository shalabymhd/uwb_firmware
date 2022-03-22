import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM2", log=False, verbose=False)
# uwb2 = UwbModule("/dev/ttyACM1", log=True, verbose=True)

id = uwb1.get_id()
uwb1.output(id)

# id = uwb2.get_id()
# uwb2.output(id)

uwb1.toggle_passive(toggle=True)

# Passive-listening callback.
def cb_timestamps(ts1,ts2,ts3,ts4):
    dict = {"tx1":ts1, "rx1":ts2, "tx2":ts3, "rx2":ts4}
    uwb1.output(dict)

# Slave measurement reading callback. 
def cb_slave(neighbour_id,meas,tx1,rx1,tx2,rx2,tx3,rx3):
    if neighbour_id != uwb1.id:
        dict = {"neighbour":neighbour_id, "range":meas}
        uwb1.output(dict) 

uwb1.register_callback("R99",cb_timestamps)

# TODO: this ends up saving some data twice. Remove for now
# uwb1.register_callback("R05",cb_slave)

while True:
    data1 = uwb1.do_twr(1,mult_twr=1,output_ts=True,meas_at_target=False)
    uwb1.output(data1)
    time.sleep(0.01)

    # data2 = uwb2.do_twr(1,mult_twr=2,meas_at_target=False)
    # uwb2.output(data2) 
    # time.sleep(0.01)