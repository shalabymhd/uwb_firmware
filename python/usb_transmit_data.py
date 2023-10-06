import time
from pyuwb import UwbModule
"""
This script publishes a message to the USB port continuously until terminated.
"""

uwb1 = UwbModule("/dev/ttyACM1", log=False, verbose=False)
uwb2 = UwbModule("/dev/ttyACM2", log=False, verbose=False)
uwb3 = UwbModule("/dev/ttyACM3", log=False, verbose=False)

id1 = uwb1.get_id()
uwb1.output(id1)

id2 = uwb2.get_id()
uwb2.output(id2)

id3 = uwb3.get_id()
uwb3.output(id3)

uwb3.set_passive_listening()

# Passive-listening callback.
def cb_passive(data):
    dict = {"initiator_id":data[0], "target_id":data[1],
            "rx_ts1":data[2], "rx_ts2":data[3], "rx_ts3":data[4],
            "tx_ts1_n":data[5], "rx_ts1_n":data[6],
            "tx_ts2_n":data[7], "rx_ts2_n":data[8],
            "tx_ts3_n":data[9], "rx_ts3_n":data[10],
            "fpp1":data[11], "fpp2":data[12], "fpp3":data[13],
            "rxp1":data[14], "rxp2":data[15], "rxp3":data[16],
            "std1":data[17], "std2":data[18], "std3":data[19],
            "fpp1_n":data[20], "fpp2_n":data[21],
            "rxp1_n":data[22], "rxp2_n":data[23],
            "std1_n":data[24], "std2_n":data[25]}
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
            "fpp1": data[8],
            "fpp2": data[9],
            "rxp1": data[10],
            "rxp2": data[11],
            "std1": data[12],
            "std2": data[13]}
    uwb2.output(dict) 

uwb3.register_listening_callback(cb_passive)
uwb2.register_range_callback(cb_target)

counter = 0
while True:
    data1 = uwb1.do_twr(id2['id'],ds_twr=True,meas_at_target=True)
    uwb1.output(data1)
    # time.sleep(0.01)

    data2 = uwb2.do_twr(id3['id'],ds_twr=True,meas_at_target=True)
    uwb2.output(data2) 
    # time.sleep(0.01)

    data3 = uwb3.do_twr(id1['id'],ds_twr=True,meas_at_target=True)
    uwb2.output(data3) 
    # time.sleep(0.01)

    # uwb3.wait_for_messages()

    counter += 1
    print(counter)
