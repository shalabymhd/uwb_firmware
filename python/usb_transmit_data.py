import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM1", log=False, verbose=False)
uwb2 = UwbModule("/dev/ttyACM2", log=False, verbose=False)
uwb3 = UwbModule("/dev/ttyACM3", log=False, verbose=False)

id = uwb1.get_id()
uwb1.output(id)

id = uwb2.get_id()
uwb2.output(id)

id = uwb3.get_id()
uwb3.output(id)

uwb3.toggle_passive(toggle=True)

# Passive-listening callback.
def cb_passive(initiator_id, target_id,
               rx_ts1,rx_ts2,rx_ts3,
               tx_ts1_n,rx_ts1_n,
               tx_ts2_n,rx_ts2_n,
               tx_ts3_n,rx_ts3_n,
               Pr1,Pr2,Pr3,
               Pr1_n,Pr2_n):
    dict = {"initiator_id":initiator_id, "target_id":target_id,
            "rx_ts1":rx_ts1, "rx_ts2":rx_ts2, "rx_ts3":rx_ts3,
            "tx_ts1_n":tx_ts1_n, "rx_ts1_n":rx_ts1_n,
            "tx_ts2_n":tx_ts2_n, "rx_ts2_n":rx_ts2_n,
            "tx_ts3_n":tx_ts3_n, "rx_ts3_n":rx_ts3_n,
            "Pr1":Pr1, "Pr2":Pr2, "Pr3":Pr3,
            "Pr1_n":Pr1_n, "Pr2_n":Pr2_n}
    uwb3.output(dict)

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

uwb3.register_callback("S01",cb_passive)
# uwb2.register_callback("S05",cb_target)

counter = 0
while True:
    data1 = uwb1.do_twr(2,mult_twr=1,meas_at_target=True)
    # uwb1.output(data1)
    # time.sleep(0.01)

    # data2 = uwb2.do_twr(1,mult_twr=2,meas_at_target=False)
    # uwb2.output(data2) 
    # time.sleep(0.01)

    counter += 1
    print(counter)
