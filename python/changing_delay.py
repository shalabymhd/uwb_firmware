import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
"""
This script publishes a message to the USB port continuously until terminated.
"""

ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[0], log=False, verbose=False)
uwb2 = UwbModule(ports[1], log=False, verbose=False)

id1 = uwb1.get_id()
uwb1.output(id1)

id2 = uwb2.get_id()
uwb2.output(id2)

target_id = id2['id']
meas_at_target = False
mult_twr = True

command_key = "C05"
command_key = command_key.encode(uwb1._encoding)
msg = uwb1.packer.pack([target_id, meas_at_target, mult_twr],
                       uwb1._c_format_dict[command_key])
msg = command_key + msg

n = 10000
data = np.zeros(n,)

def save_data(array, delay, T):
    filename = 'datasets/' + str(delay) + '_' + str(int(T*1e9)) + '.txt'
    np.savetxt(filename, array, fmt='%2.4f')

time.sleep(10)

counter = 0
delay = 700
uwb2.set_response_delay(delay)
t_start = time.time()
while True:

    uwb1.device.write(msg)
    out = uwb1._read() 
    try:
        data[counter] = out[6:12]
    except:
        pass

    counter+=1
    if counter > n:
        t_end = time.time()
        T = t_end - t_start
        
        counter = 0

        time.sleep(2)
        save_data(data,delay,T)

        data *= 0

        delay += 10
        uwb2.set_response_delay(delay)
        
        time.sleep(2)

        if delay>6500:
            break

        print(delay)

        t_start = time.time()


