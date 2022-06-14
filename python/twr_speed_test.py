import time
from pyuwb import UwbModule, find_uwb_serial_ports
"""
Evaluates the frequency of continuous TWR ranging, with and without the python
interface.
"""
ports = find_uwb_serial_ports()
port = ports[0]
target_id = 6
num_trials = 100

# Use the actual python interface
uwb1 = UwbModule(port, verbose=True)
counter = 0
start_time = time.time()
for i in range(num_trials):
    data = uwb1.do_twr(target_id, mult_twr=True, meas_at_target=True)
    print(counter)
    counter += 1


freq_with_python = 1/((time.time() - start_time)/num_trials)

uwb1.close() # shuts down the internal threads.
time.sleep(1)

# Just send raw command directly. Skips the interfaces' packing/unpacking 
# procedures.
counter = 0
start_time = time.time()
command = b"C05|" + str(target_id).encode() + b"|1|1\r"
for i in range(num_trials):
    uwb1.device.write(command)
    resp = uwb1.device.readline()
    print(resp)
    print(counter)
    counter += 1
freq_without_python = 1/((time.time() - start_time)/num_trials)


print("Average frequency with python interface:")
print(str(freq_with_python) + "Hz")

print("Average frequency without python interface:")
print(str(freq_without_python) + "Hz")

