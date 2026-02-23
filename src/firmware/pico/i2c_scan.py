import machine

# 初始化 I2C0 (GP4=SDA, GP5=SCL)
i2c = machine.I2C(0, sda=machine.Pin(4), scl=machine.Pin(5), freq=100000)

print("--- 开始扫描 Pico I2C0 总线 ---")
devices = i2c.scan()

if not devices:
    print("❌ 未发现任何设备！请检查接线、共地和电源。")
else:
    print(f"✅ 发现 {len(devices)} 个设备:")
    for device in devices:
        addr_hex = hex(device)
        # 匹配已知地址
        info = ""
        if device == 0x40: info = "(INA260 电流传感器)"
        elif device == 0x36: info = "(AS5600 编码器)"
        elif device == 0x4A or device == 0x4B: info = "(BNO085 IMU)"
        
        print(f" - 地址: {addr_hex} {info}")
print("--- 扫描结束 ---")
