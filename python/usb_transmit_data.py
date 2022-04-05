import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM1", log=False, verbose=False)
uwb2 = UwbModule("/dev/ttyACM2", log=False, verbose=False)

id = uwb1.get_id()
uwb1.output(id)

id = uwb2.get_id()
uwb2.output(id)

uwb1.toggle_passive(toggle=True)

# Passive-listening callback.
def cb_passive(ts1,ts2,ts3,ts4):
    dict = {"tx1":ts1, "rx1":ts2, "tx2":ts3, "rx2":ts4}
    uwb1.output(dict)

# Target measurement reading callback. 
def cb_target(neighbour_id,meas,tx1,rx1,tx2,rx2,tx3,rx3,Pr1,Pr2):
    dict = {"neighbour": neighbour_id,
            "range": meas,
            "tx1": tx1,
            "rx1": rx1,
            "tx2": tx2,
            "rx2": rx2,
            "tx3": tx3,
            "rx3": rx3,
            "Pr1": Pr1,
            "Pr2": Pr2}
    uwb2.output(dict) 

uwb2.register_callback("S01",cb_passive)
uwb2.register_callback("S05",cb_target)

counter = 0
while True:
    data1 = uwb1.do_twr(2,mult_twr=1,meas_at_target=False)
    uwb1.output(data1)
    # time.sleep(0.001)

    # data2 = uwb2.do_twr(1,mult_twr=2,meas_at_target=False)
    # uwb2.output(data2) 
    # time.sleep(0.01)

    counter += 1
    print(counter)
