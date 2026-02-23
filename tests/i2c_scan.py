"""
i2c_scan.py - Low-level hardware scanner.
Bypass all SH2 drivers to verify physical I2C presence.
"""
from machine import I2C, Pin
import time

def scan():
    # Matches your wiring: SDA=GP12, SCL=GP13
    # Note: GP12/13 belongs to I2C0
    i2c = I2C(0, sda=Pin(12), scl=Pin(13), freq=100000) # Start slow for stability
    
    print("🔍 Scanning I2C bus...")
    devices = i2c.scan()
    
    if not devices:
        print("❌ No I2C devices found! Check power and wiring.")
    else:
        print(f"✅ Found {len(devices)} device(s): {[hex(d) for d in devices]}")
        # BNO085 default is 0x4a or 0x4b
        if 0x4a in devices or 0x4b in devices:
            print("🚀 BNO085 is detected on the bus!")

if __name__ == "__main__":
    scan()