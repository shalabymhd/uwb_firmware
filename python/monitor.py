import serial
ser = serial.Serial('/dev/ttyACM0', 19200, timeout=1)
ser.flushInput()

while True:
    ser_bytes = ser.read()
    decoded_bytes = ser_bytes[0:len(ser_bytes)-2].decode("utf-8")
    print(decoded_bytes)
    