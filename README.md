# Project BraveHart V2
**A Distributed Robotics Perception & Control System**

## 📖 Description
**Project BraveHart V2** is a modular robotics platform developed on the Picar-X chassis. It utilizes a **"Brain-Cerebellum" (大脑与小脑)** architecture to ensure high-level intelligence and low-level real-time responsiveness.

The system decouples high-level task planning (ROS 2) from deterministic sensor data acquisition (Pico C++ SDK), connected via a robust USB CDC serial bridge.



---

## 🏗️ System Architecture
- **Host (Brain)**: Raspberry Pi 3B running **ROS 2 Humble**. Responsible for SLAM, navigation, and motor command execution via the Robot Hat.
- **Microcontroller (Cerebellum)**: Raspberry Pi Pico. Acts as a high-speed sensor hub managing I2C, GPIO, and PWM signals.
- **Communication**: Bidirectional data flow via USB Serial, ensuring low-latency telemetry and command execution.

---

## 🔌 Hardware Configuration

### I2C Network (TCA9548A Multiplexer)
| Channel | Device | Address | Function |
| :--- | :--- | :--- | :--- |
| **Main** | TCA9548A | `0x70` | I2C Hub / Bus Switch |
| **CH 0** | BNO085 | `0x4A` | 9-DoF Orientation (IMU) |
| **CH 1** | INA260 | `0x40` | Power & Current Monitor (Planned) |

### GPIO Pinout (Pico)
| Pin | Function | Device | Notes |
| :--- | :--- | :--- | :--- |
| **GP4/5** | I2C0 SDA/SCL | TCA9548A | Main Communication Bus |
| **GP16** | Reset (RST) | TCA9548A | Hardware Reset Line |
| **GP17** | Interrupt (INT) | BNO085 | Data Ready Signal (Active Low) |
| **GP22** | Reset (RST) | BNO085 | Sensor Hardware Reset |
| **GP10/11** | Encoder A/B | Left Motor | Pulse Capture (Interrupt) |
| **GP12/13** | Encoder A/B | Right Motor | Pulse Capture (Interrupt) |
| **GP14/15** | Trig/Echo | HC-SR04 | Ultrasonic Ranging (Needs Voltage Divider) |



---

## 🚀 Deployment Workflow
The project features a **Remote Cross-Platform Deployment** pipeline:

1. **Development**: Write C++ (Pico SDK) or Python (ROS 2) on the **Ubuntu P620 Workstation**.
2. **Build**: Cross-compile for ARM/Pico architecture locally in the `build/` directory.
3. **Deploy**: Use `./scripts/deploy.sh` to push firmware to the **Raspberry Pi 3B** via SSH, which then flashes the **Pico** using `picotool`.

```bash
# Example Deployment Command
./scripts/deploy.sh

## Tech Stack
Languages: C++, Python, CMake

Frameworks: ROS 2 Humble, Raspberry Pi Pico SDK

Protocols: I2C (with Multiplexing), SHTP (for BNO085), USB CDC Serial

Simulation: Wokwi (Custom Chip Modeling for TCA9548A & BNO085)

## Maintenance
Author: Tim Yi

Last Updated: 2026-02-23

Status: Active Development (Phase 2: Sensor Integration)