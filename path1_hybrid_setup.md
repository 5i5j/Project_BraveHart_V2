# Project Specification: Path 1 (Hybrid Integration)

## 1. System Architecture
- **Master Controller:** Raspberry Pi 3B (Running ROS 2 Humble)
- **Sensor Hub:** Raspberry Pi Pico (Connected via USB-Serial)
- **Power & Actuation:** Robot Hat (Mounted on Pi 3B)

## 2. Hardware Connections
- **BNO085 IMU:** Connected to Pico via I2C interface.
- **Drive Motors (Rear):** Red/Black power wires connected to Motor A/B ports on **Robot Hat**.
- **Motor Encoders:** Signal wires (A/B phases) connected to **Pico** GPIOs (for high-frequency pulse counting using PIO).
- **Steering Motor (Clutch Gear/Servo):** 3-wire PWM interface connected to **Robot Hat** Servo/PWM ports.
- **Ultrasonic sensor:** connected to Pico
- **Power Supply:** 7.4V Battery connected to Robot Hat (Pi 3B powered via Hat).

## 3. Software Responsibilities
### A. Raspberry Pi 3B (main.cpp)
- **Node Type:** ROS 2 Node (Publisher/Subscriber).
- **Serial Client:** Listen to `/dev/ttyACM0` for BNO085 and Encoder data from Pico.
- **I2C Master:** Send PWM commands to Robot Hat to control motors and steering.
- **Data Logger:** Record telemetry into **Parquet** format for S3 upload.

### B. Raspberry Pi Pico (pico_firmware.cpp)
- **IMU Handler:** Sample BNO085 orientation at 50Hz.
- **Encoder Handler:** Use hardware interrupts to track motor ticks.
- **Serial Bridge:** Package data into a binary frame and send to Pi 3B.