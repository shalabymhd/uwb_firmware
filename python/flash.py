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
    ser.write("C09\r".encode())
    response = ser.readline() + ser.read(ser.in_waiting)
    print(response)
    ser.close()
    time.sleep(1)

    if response == b"R09\r\n":
        # If valid response was obtained, flash firmware.

        # First we will add a "DFU suffix" which specifies the vendor/product ID
        # of the STM32F405 so that we are protected against flashing to something 
        # else.
        suffix_command = "dfu-suffix --vid 0483 --pid df11 --add ./build/firmware.bin"
        print(">> " + suffix_command)
        os.system(suffix_command)

        # Upload the new firmware.
        upload_command = "dfu-util --device 0483:5740,0483:df11 -a 0 --dfuse-address 0x08000000:leave -D ./build/firmware.bin"
        print(">> " + upload_command)
        os.system(upload_command)

    else:
        print("Did not receive expected response."
        + "Device is not a valid UWB Module or "
        + "there may be a problem jumping to the bootloader.")
        
   


