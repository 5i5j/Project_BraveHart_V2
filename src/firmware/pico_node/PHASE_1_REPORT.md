# Experiment Report: IMU Jitter Analysis (Phase 1)
**Date:** 2026-02-27
**Target Hardware:** Raspberry Pi Pico (Dual-Core) + BNO085 (I2C)
**Host:** tim-pi-4b

## 1. Objective
Identify the source of timing jitter in BNO085 orientation data (50Hz) and verify if `printf` on Core 0 interferes with Core 1's sensor acquisition.

## 2. Experimental Setup
- **Core 1:** Dedicated to BNO085 driver (`sh2_service`) and I2C communication (100kHz).
- **Core 0:** Handles statistics and serial output via USB CDC.
- **Sampling Rate:** 50Hz (Target Period: 20,000 us).
- **Silent Mode:** Core 0 remains silent for 10 seconds, buffering 600 samples into RAM before outputting.

## 3. Results & Observations
| Metric | Value |
| :--- | :--- |
| **Total Samples** | 600 |
| **Max Jitter** | 1505 us |
| **Min Jitter** | -1836 us |
| **Avg Jitter** | -53.11 us |

### Key Findings:
1. **Clock Drift:** The consistent negative average jitter (-53.11 us) indicates a slight clock frequency mismatch between the Pico's crystal and the BNO085's internal oscillator.
2. **Compensatory Jitter Pattern:** The raw data shows a "High Positive followed by High Negative" pattern (e.g., +1076 us then -1266 us). This suggests that even when the driver is delayed, the BNO085 buffers the next frame, which is then retrieved immediately.
3. **Internal Bottlenecks:** Despite "Silent Mode" on Core 0, jitters > 1.5ms still occur. This confirms that `printf` is not the sole cause; the 100kHz I2C bus speed or internal `sh2` processing tasks on Core 1 are likely contributors.

## 4. Conclusion & Next Steps
- **Hypothesis Correction:** While `printf` adds noise, the core jitter originates from I2C bus latency or driver polling efficiency.
- **Next Action:** Upgrade I2C to 400kHz and implement GPIO Interrupt (INT pin) to trigger `sh2_service` more precisely.

## 5. Git Commands
tim@tim-Ubuntu22-P620:~/Tim/Project_BraveHart_V2$ ls src/firmware/pico_node/
CMakeLists.txt  FreeRTOSConfig.h  gemini_main.cpp  PHASE_1_REPORT.md  pico_main.cpp
tim@tim-Ubuntu22-P620:~/Tim/Project_BraveHart_V2$ git add src/firmware/pico_node/.
tim@tim-Ubuntu22-P620:~/Tim/Project_BraveHart_V2$ git commit -m "Phase 1: Added silent jitter test code and experimental report"

## 6. Post-Optimization Results (Fast Mode + Interrupt)
**Changes:** Switched I2C to 400kHz; Implemented GPIO IRQ on INT pin.

| Metric | Optimized Value |
| :--- | :--- |
| **Max Jitter** | 593 us |
| **Avg Jitter** | -53.30 us |

**Conclusion:** The optimization reduced peak jitter by over 60%. The system is now significantly more deterministic. Remaining jitter is likely intrinsic to the BNO085 internal processing and I2C transaction timing. We are now ready for Phase 2.

