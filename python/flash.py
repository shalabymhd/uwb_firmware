import serial
from serial.tools import list_ports
import os
import time
ports = list_ports.comports()

if len(ports) == 0:
    print("No COM ports detected.")

for port in ports:
    ser = serial.Serial(port.device, 19200, timeout=1)

    # Send USB command to jump to bootloader
    ser.write("C08\r".encode())
    response = ser.readline() + ser.read(ser.in_waiting)
    print(response)
    ser.close()
    time.sleep(1)

    if response == b"R08\r\n":
        # If valid response was obtained, flash firmware.
        upload_command = "dfu-util -a 0 --dfuse-address 0x08000000:leave -D ./build/firmware.bin"
        print(">> " + upload_command)
        os.system(upload_command)
        
    else:
        print("Did not receive expected response."
        + "Device is not a valid UWB Module or "
        + "there may be a problem jumping to the bootloader.")
        
   


