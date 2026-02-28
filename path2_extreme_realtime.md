# Project Specification: Path 2 (Extreme Real-time Architecture)

## 1. System Architecture
- **Intelligence Layer:** Raspberry Pi 3B (ROS 2, S3, High-level Navigation).
- **Control Layer (Real-time):** Raspberry Pi Pico (Low-level PID, PWM, Sensing).
- **Power System:** 5V 5A UBEC + 1000uF Electrolytic Capacitor.

## 2. Hardware Connections (The "Decoupled" Method)
- **Power Distribution:**
    - Battery (7.4V) -> **DRV8833** VCC (Main Drive Power).
    - Battery (7.4V) -> **UBEC (5V 5A)** -> Pi 3B, Pico, Servo, and BNO085.
    - **Capacitor:** 1000uF across DRV8833 VCC and GND to suppress voltage spikes.
- **Actuation:**
    - **DRV8833:** Controlled by **Pico** GPIOs (PWM for velocity/direction).
    - **Steering Servo:** PWM Signal wire connected to **Pico** GPIO (Power from UBEC 5V).
- **Feedback:**
    - **BNO085 & Encoders:** All connected to **Pico**.

## 3. Software Responsibilities
### A. Raspberry Pi 3B (main.cpp)
- **Role:** High-level supervisor.
- **Communication:** Send target velocity ($v$) and steering angle ($\delta$) to Pico via USB-Serial.
- **Data Lake:** Handle S3 synchronization and Parquet serialization.

### B. Raspberry Pi Pico (pico_firmware.cpp)
- **Hard Real-time Controller:** Executes the **PID Control Loop** (Comparing Target vs. Encoder Feedback).
- **Safety Logic:** Implement emergency stop if BNO085 detects abnormal tilt or collision.
- **Feedback loop:** Send fully fused Odometry data back to Pi 3B.