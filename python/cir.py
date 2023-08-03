# %%
from pyuwb import UwbModule, find_uwb_serial_ports
import time

ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[1], verbose=True, timeout=0.1)
uwb2 = UwbModule(ports[0], verbose=True, timeout=0.1)

id1 = uwb1.get_id()
id2 = uwb2.get_id()

uwb1.register_cir_callback(lambda x: print("CIR 1: ", x))
uwb2.register_cir_callback(lambda x: print("CIR 2: ", x))
# uwb1.register_cir_callback(lambda x: print("CIR 1 Received"))
# uwb2.register_cir_callback(lambda x: print("CIR 2 Received"))

start = time.time()
n = 10
i = 0
while i < n:
    uwb2.do_twr(
        target_id = id1['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )

    # time.sleep(0.2)
    # print("meow1")
    uwb1.wait_for_messages()
    # uwb2.wait_for_messages()
    # print("meow2")

    uwb1.do_twr(
        target_id = id2['id'],
        mult_twr = True,
        meas_at_target=True,
        get_cir=True,
    )
    
    # uwb1.wait_for_messages()
    # print("meow3")
    uwb2.wait_for_messages()
    # print("meow4")
    # time.sleep(0.1)

    # data = uwb1.get_cir()

    # time.sleep(0.02)

    # data = uwb2.get_cir()
    
    # time.sleep(0.1)

    # print(data)
    i += 1

print("Freq: ", 2 * n / (time.time() - start))
# %%