import time
from pyuwb import UwbModule, find_uwb_serial_ports
import numpy as np
import random
"""
This script publishes a message to the USB port continuously until terminated.
"""

ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[1], log=False, verbose=False)
uwb2 = UwbModule(ports[0], log=False, verbose=False)

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

n = 2500
data = np.zeros(n,)

def save_data(array, delay, true_delay, T):
    filename = 'datasets/' + str(delay) + '_' + str(true_delay) \
                + '_' + str(int(T*1e9)) + '.txt'
    np.savetxt(filename, array, fmt='%2.4f')

time.sleep(2)

counter = 0
delay = 700
uwb2.set_response_delay(delay)

time.sleep(2)

t_start = time.time()
while True:

    uwb1.device.write(msg)
    out = uwb1._read() 
    try:
        out = str(out).split('|')
        data[counter] = out[2]
    except:
        pass

    counter+=1
    if counter > n:
        t_end = time.time()
        T = t_end - t_start

        try:
            true_delay = (int(out[8]) - int(out[6]))*(1.0 / 499.2e6 / 128.0)*1e6
            if true_delay<0:
                true_delay += 2**32*(1.0 / 499.2e6 / 128.0)*1e6
            true_delay = int(true_delay)
        except:
            true_delay = -1
        
        counter = 0

        save_data(data,delay,true_delay,T)

        data *= 0

        # delay += 10
        delay = random.randint(400,6500)

        uwb2.set_response_delay(delay)

        if delay>6500:
            break

        print(delay)

        time.sleep(1)

        t_start = time.time()


