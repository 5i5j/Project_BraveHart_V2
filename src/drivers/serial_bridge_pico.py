# src/drivers/serial_bridge_pico.py
import serial
import struct

class SerialBridgePico:
    def __init__(self, port='/dev/ttyACM0', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=0.01)
        # Using a very strict format string
        self.fmt = '<IIfffffff'
        self.packet_size = struct.calcsize(self.fmt) # Must be 36
        self.ser.reset_input_buffer()

    def read_packet(self):
        if self.ser.in_waiting < (self.packet_size + 1):
            return None

        # Look for Header 'D'
        char = self.ser.read(1)
        if char != b'D':
            return None

        payload = self.ser.read(36)
        if len(payload) != 36:
            return None

        # UNPACKING CHECK
        data = struct.unpack(self.fmt, payload)
        
        # DEBUG PRINT: Just once to see the raw values
        # If seq is 0 and pico_us is 65536, we have an alignment issue
        # print(f"DEBUG: seq={data[0]}, ts={data[1]}, first_val={data[2]}")
        
        return data