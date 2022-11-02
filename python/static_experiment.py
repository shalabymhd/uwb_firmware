from pyuwb import UwbModule, find_uwb_serial_ports
import time

timestr = time.strftime("%Y%m%d-%H%M%S")

ports = find_uwb_serial_ports()

uwb1 = UwbModule(ports[0])
uwb2 = UwbModule(ports[1])

id1 = uwb1.get_id()['id']
id2 = uwb2.get_id()['id']

while True:
    data = uwb1.do_twr(
        target_id = id2,
        mult_twr = True,
    )
    t = time.time()
    with open(timestr+".txt", "a") as myfile:
        myfile.write(str(t) + "," + str(data) + "\n")