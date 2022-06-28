import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM1", log=False, verbose=False)
uwb2 = UwbModule("/dev/ttyACM2", log=False, verbose=False)
# uwb3 = UwbModule("/dev/ttyACM3", log=False, verbose=False)

id = uwb1.get_id()
uwb1.output(id)

id = uwb2.get_id()
uwb2.output(id)

# id = uwb3.get_id()
# uwb3.output(id)

# uwb3.set_passive_listening()

# Passive-listening callback.
def cb_passive(data):
    dict = {"initiator_id":data[0], "target_id":data[1],
            "rx_ts1":data[2], "rx_ts2":data[3], "rx_ts3":data[4],
            "tx_ts1_n":data[5], "rx_ts1_n":data[6],
            "tx_ts2_n":data[7], "rx_ts2_n":data[8],
            "tx_ts3_n":data[9], "rx_ts3_n":data[10],
            "Pr1":data[11], "Pr2":data[12], "Pr3":data[13],
            "Pr1_n":data[14], "Pr2_n":data[15]}
    uwb3.output(dict)

# Target measurement reading callback. 
def cb_target(data):
    dict = {"neighbour": data[0],
            "range": data[1],
            "tx1": data[2],
            "rx1": data[3],
            "tx2": data[4],
            "rx2": data[5],
            "tx3": data[6],
            "rx3": data[7],
            "Pr1": data[8],
            "Pr2": data[9]}
    uwb2.output(dict) 

# uwb3.register_listening_callback(cb_passive)
uwb2.register_range_callback(cb_target)

counter = 0
while True:
    data1 = uwb1.do_twr(id['id'],mult_twr=True,meas_at_target=True)
    uwb1.output(data1)
    # time.sleep(0.01)

    # data2 = uwb2.do_twr(5,mult_twr=1,meas_at_target=True)
    # uwb2.output(data2) 
    # time.sleep(0.01)

    counter += 1
    print(counter)
