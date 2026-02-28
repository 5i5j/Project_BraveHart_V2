# Engineering Report: BNO085 Sampling Frequency Optimization

**Date:** 2026-02-27  
**Project:** PiCar ROS 2 Base (High-Performance Robotics)  
**Hardware:** Raspberry Pi Pico (RP2040), BNO085 (IMU via I2C)

---

## 1. Executive Summary
This report documents the empirical testing of the BNO085 IMU across various frequencies to determine the "Sweet Spot" for a differential drive robot. The goal is to balance high-frequency responsiveness with system-level determinism.

## 2. Experimental Data
All tests were conducted using **1,000 samples** on a dedicated core (Core 1) with I2C running at **400kHz**.

| Target Frequency | Target Period ($T_{target}$) | Avg Jitter | Max/Min Jitter | Jitter Ratio (%) | Result |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **50 Hz** | 20,000 µs | -53.31 µs | ± 0.6 ms | ~3.0% | **Very Stable** |
| **100 Hz** | 10,000 µs | -27.05 µs | ± 2.0 ms | ~20.0% | **Optimal** |
| **200 Hz** | 5,000 µs | -13.33 µs | ± 1.8 ms | ~38.0% | **Stress Limit** |
| **66.7 Hz** | 15,000 µs | ~ -5,000 µs | N/A | N/A | **Failed (Fallback)** |

---

## 3. Critical Discovery: The "15ms Fallback Trap"
A significant finding occurred during the **15ms (66.7Hz)** test. The sensor exhibited a constant **-5ms bias**, consistently delivering data every 10ms regardless of the 15ms request.

* **Analysis**: BNO085 firmware utilizes **discrete scheduling slots**. 
* **Conclusion**: 15ms is not a native harmonic for the internal fusion engine. The sensor fallbacks to the nearest stable frequency (10ms). **Engineering Rule**: Always align system timing with hardware-native multiples (5ms/10ms/20ms).

---

## 4. Engineering Trade-off Analysis

### 🟢 100Hz: The "Sweet Spot" (Recommended)
- **Responsiveness**: 10ms latency is ideal for Heading (Yaw) control in ground vehicles.
- **CPU Headroom**: Leaves ~50% of the cycle time for Encoder Interrupts (ISR) and Odometry calculations.
- **Manageability**: A 20% peak jitter is easily compensated by using dynamic $dt$ in the ROS 2 EKF node.

### 🟡 200Hz: The "Limit Test"
- **Performance**: Lowest `Avg Jitter`, but at the cost of extreme timing variance (38%).
- **Risk**: The I2C bus and SH2 protocol stack have zero idle time, increasing the risk of race conditions when motor PWM and high-speed Encoders are active.

---

## 5. Final Decision
**Selected Operating Frequency: 100 Hz (`TARGET_PERIOD_US 10000`)**

### Implementation Strategy:
1.  **Time-Stamping**: Instead of assuming a fixed 0.01s interval, the system must calculate:
    $dt = (Time_{now} - Time_{prev}) \times 10^{-6}$
2.  **Core Allocation**: Core 1 remains dedicated to IMU and Encoder ISRs to minimize context-switching latency.
3.  **Future-Proofing**: The 100Hz baseline provides a stable foundation for Phase 2 (Encoder Integration) and Phase 3 (ROS 2 Navigation Stack).

---
**Status:** ✅ **Approved for Phase 2 Integration**