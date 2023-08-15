# %%
from pyuwb import UwbModule, find_uwb_serial_ports
import time

ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[0], verbose=True, timeout=0.1)
uwb2 = UwbModule(ports[1], verbose=True, timeout=0.1)
uwb3 = UwbModule(ports[2], verbose=True, timeout=0.1)
# uwb4 = UwbModule(ports[3], verbose=True, timeout=0.1)

id1 = uwb1.get_id()
id2 = uwb2.get_id()
id3 = uwb3.get_id()
# id4 = uwb4.get_id()

# if id1['id'] == 9:
#     uwb3, uwb1 = uwb1, uwb3
#     id3, id1 = id1, id3
# elif id2['id'] == 9:
#     uwb3, uwb2 = uwb2, uwb3
#     id3, id2 = id2, id3

def listening_callback(data, my_id):
    if data[2] != 0: 
        # Data is valid
        msg = {}
        msg['rx1'] = data[2]
        msg['rx2'] = data[3]
        msg['rx3'] = data[4]
        msg['tx1_n'] = data[5]
        msg['rx1_n'] = data[6]
        msg['tx2_n'] = data[7]
        msg['rx2_n'] = data[8]
        msg['tx3_n'] = data[9]
        msg['rx3_n'] = data[10]
        msg['fpp1'] = data[11]
        msg['fpp2'] = data[12]
        msg['fpp3'] = data[13]
        msg['skew1'] = data[14]
        msg['skew2'] = data[15]
        msg['skew3'] = data[16]
        msg['fpp1_n'] = data[17]
        msg['fpp2_n'] = data[18]
        msg['skew1_n'] = data[19]
        msg['skew2_n'] = data[20]
        print(my_id, msg)

uwb1.register_cir_callback(lambda x: print("CIR 1: ", x))
uwb2.register_cir_callback(lambda x: print("CIR 2: ", x))
uwb3.register_cir_callback(lambda x: print("CIR 3: ", x))
# uwb4.register_cir_callback(lambda x: print("CIR 4: ", x))

uwb1.toggle_passive(True)
uwb2.toggle_passive(True)
uwb3.toggle_passive(True)
# uwb4.toggle_passive(True)

uwb1.register_listening_callback(listening_callback, id1['id'])
uwb2.register_listening_callback(listening_callback, id2['id'])
uwb3.register_listening_callback(listening_callback, id3['id'])
# uwb4.register_listening_callback(listening_callback, id4['id'])

start = time.time()
n = 4
i = 0
while i < n:
    data = uwb2.do_twr(
        target_id = id1['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )

    uwb1.wait_for_messages(timeout=0.1)
    uwb2.wait_for_messages(timeout=0.1)
    uwb3.wait_for_messages(timeout=0.1)
    # uwb4.wait_for_messages(timeout=0.1)

    
    # time.sleep(0.1)
    print(data)

    data = uwb3.do_twr(
        target_id = id2['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )

    uwb1.wait_for_messages(timeout=0.1)
    uwb2.wait_for_messages(timeout=0.1)
    uwb3.wait_for_messages(timeout=0.1)
    # uwb4.wait_for_messages(timeout=0.1)
    
    # time.sleep(0.1)
    print(data)

    data = uwb1.do_twr(
        target_id = id2['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )

    uwb1.wait_for_messages(timeout=0.1)
    uwb2.wait_for_messages(timeout=0.1)
    uwb3.wait_for_messages(timeout=0.1)
    # uwb4.wait_for_messages(timeout=0.1)

    # # time.sleep(0.1)
    print(data)

    data = uwb1.do_twr(
        target_id = id3['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )

    uwb1.wait_for_messages(timeout=0.1)
    uwb2.wait_for_messages(timeout=0.1)
    uwb3.wait_for_messages(timeout=0.1)
    # uwb4.wait_for_messages(timeout=0.1)

    # data = uwb1.do_twr(
    #     target_id = id4['id'],
    #     mult_twr = True,
    #     meas_at_target=True,
    #     get_cir=True,
    # )

    # uwb1.wait_for_messages(timeout=0.1)
    # uwb2.wait_for_messages(timeout=0.1)
    # uwb3.wait_for_messages(timeout=0.1)
    # uwb4.wait_for_messages(timeout=0.1)
    
    i += 1

print("Freq: ", 4*n / (time.time() - start))
# %%