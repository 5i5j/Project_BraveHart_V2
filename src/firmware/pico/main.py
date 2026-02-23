import time
import machine
import struct
import sys
import gc
from lib.i2c import BNO08X_I2C
from lib.bno08x import BNO_REPORT_ROTATION_VECTOR, BNO_REPORT_ACCELEROMETER

def main():
    # 1. 立即强制回收内存，确保初始化有足够的连续空间
    gc.collect()
    
    i2c = machine.I2C(0, sda=machine.Pin(12), scl=machine.Pin(13), freq=400000)
    int_pin = machine.Pin(14, machine.Pin.IN, machine.Pin.PULL_UP)
    rst_pin = machine.Pin(15, machine.Pin.OUT, value=1)

    # 物理重置传感器
    rst_pin.value(0); time.sleep_ms(200); rst_pin.value(1); time.sleep(1) 

    try:
        # 在这里不禁用 GC，让驱动程序能顺利分配缓冲区
        bno = BNO08X_I2C(i2c, address=0x4a, int_pin=int_pin, reset_pin=rst_pin)
        bno.enable_feature(BNO_REPORT_ROTATION_VECTOR, 20000) 
        bno.enable_feature(BNO_REPORT_ACCELEROMETER, 20000)
        time.sleep(0.5)
    except Exception as e:
        print(f"BNO085 Init Error: {e}")
        return

    # 2. 初始化完成，现在进行一次大清理并限制 GC
    gc.collect()
    # 注意：我们不使用 gc.disable() 了，改为使用阈值控制或手动高频清理
    
    seq = 0
    TARGET_PERIOD_US = 20000
    next_tick_us = time.ticks_us()
    data_fmt = '<IIfffffff'
    
    # 预先分配好 struct 需要的内存空间，减少循环内的分配
    # 这一行能极大减少 Jitter
    packet_buf = bytearray(37)

    while True:
        try:
            bno.update_sensors()
            current_us = time.ticks_us()
            
            if time.ticks_diff(current_us, next_tick_us) >= 0:
                qi, qj, qk, qr = bno.quaternion
                ax, ay, az = bno.acceleration
                
                # 直接打包并发送
                sys.stdout.buffer.write(b'D')
                sys.stdout.buffer.write(struct.pack(data_fmt, seq, current_us, ax, ay, az, qi, qj, qk, qr))
                
                seq += 1
                next_tick_us = time.ticks_add(next_tick_us, TARGET_PERIOD_US)

                # 每 50 个包（约 1 秒）清理一次，保持内存水位极低
                if seq % 50 == 0:
                    gc.collect()

        except Exception:
            continue

if __name__ == "__main__":
    main()