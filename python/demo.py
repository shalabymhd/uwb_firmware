from pyuwb import UwbModule, find_uwb_serial_ports

ports = find_uwb_serial_ports()
uwb1 = UwbModule(ports[0])
uwb2 = UwbModule(ports[1], verbose=True)

id1 = uwb1.get_id()
id2 = uwb2.get_id()

while True:
    data = uwb1.do_twr(
        target_id = id2['id'],
        mult_twr = True,
    )
    uwb2.wait_for_messages()
    # print(data)