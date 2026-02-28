# Sensor Sampling Frequency Sweep Test Report

**Date:** 2026-02-27
**Robot ID:** [Your Robot ID]
**Hardware:** Raspberry Pi 4B, BNO085 IMU
**Interface:** /dev/ttyACM0 (115200 bps)

## 1. Experimental Objective
The objective of this test is to evaluate the timing stability and jitter (time difference between expected and actual sampling) of the BNO085 sensor at various frequencies (50Hz to 400Hz) to determine the optimal sampling rate for ROS 2 integration and RViz visualization.

## 2. Test Results Summary

| Target Frequency | Target Period | Avg Jitter | Max Jitter | Min Jitter | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **50 Hz** | 20,000 us | -53.77 us | 549 us | -611 us | Stable |
| **100 Hz** | 10,000 us | -28.25 us | 1,202 us | -1,256 us | **Optimal** |
| **200 Hz** | 5,000 us | -12.97 us | 1,704 us | -1,765 us | High Spikes |
| **400 Hz** | 2,500 us | -6.53 us | 1,630 us | -1,378 us | Limit Reached |

## 3. Detailed Data Analysis

### 3.1 Low Frequency (50 Hz)
At 50 Hz, the system has excessive "idle time." While the average jitter is low, the data shows consistent fluctuations around ±300 us. This is likely due to the Linux scheduler putting the process to sleep and the wake-up latency being inconsistent.

### 3.2 Medium Frequency (100 Hz)
The 100 Hz test demonstrates the most balanced performance. It provides a significant improvement in data density over 50 Hz while maintaining a predictable error margin. The periodic spikes (approx. 1.2 ms) are manageable for EKF (Extended Kalman Filter) processing.

### 3.3 High Frequency (200 Hz - 400 Hz)
At these rates, the system hits the "Real-time Wall." 
- **Latency Compensation:** The data shows massive positive jitter immediately followed by massive negative jitter. This indicates that the system is lagging and then rapidly reading the buffer to catch up.
- **CPU Load:** 400 Hz forces a context switch every 2.5 ms, which causes significant stress on the non-RT Linux kernel, leading to unpredictable timing behavior for other processes (like ROS 2 nodes).

## 4. Selection of 100 Hz as Target Frequency

We have officially selected **100 Hz** for the following reasons:

1. **Synchronization Alignment:** 100 Hz is a perfect multiple of our 200 Hz physical sampling ceiling, ensuring better alignment with the sensor's internal report interval.
2. **Oversampling Benefit:** By sampling at 100 Hz for a 50 Hz target requirement, we provide enough data for anti-aliasing filters and smoother interpolation in RViz.
3. **Resource Margin:** It leaves enough CPU "headroom" (processing capacity) to integrate Encoder data and manage Parquet file I/O without dropping frames.
4. **Deterministic Behavior:** Compared to 400 Hz, 100 Hz shows a much lower ratio of jitter-to-period, making the odometry calculation more reliable.

## 5. Next Steps
- Integrate wheel encoder pulses into the 100 Hz loop.
- Implement Parquet logging for `timestamp`, `robot_id`, `imu_quat`, and `encoder_counts`.
- Develop ROS 2 Custom Messages for RViz visualization.