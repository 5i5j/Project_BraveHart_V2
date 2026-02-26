# Hardware Wiring Diagram (RP2040)

This document defines the physical pin connections for the Project_BraveHart_V2 robotics platform.

## 1. Sensors & Modules Mapping

| Component    | Function       | Pico Pin (GPIO) | Pico Physical Pin | Signal Level | Notes                  |
|:-------------|:---------------|:----------------|:------------------|:-------------|:-----------------------|
| **BNO085**   | I2C SDA        | **GP6**         | Pin 9             | 3.3V         | I2C1 Bus               |
| **BNO085**   | I2C SCL        | **GP7**         | Pin 10            | 3.3V         | I2C1 Bus               |
| **BNO085**   | INT (Interrupt)| **GP10**        | Pin 14            | 3.3V         | Active Low             |
| **BNO085**   | RST (Reset)    | **GP11**        | Pin 15            | 3.3V         | Active Low             |
| **Encoder L**| Phase A (Green)| **GP12**        | Pin 16            | 3.3V         | PIO0 State Machine 0   |
| **Encoder L**| Phase B (Blue) | **GP13**        | Pin 17            | 3.3V         | PIO0 State Machine 0   |
| **Encoder R**| Phase A (Green)| **GP14**        | Pin 19            | 3.3V         | PIO0 State Machine 1   |
| **Encoder R**| Phase B (Blue) | **GP15**        | Pin 20            | 3.3V         | PIO0 State Machine 1   |

## 2. Power Distribution

| Source        | Target Component | Voltage | Notes                          |
|:--------- ----|:-----------------|:--------|:-------------------------------|
| Pico 3V3 (OUT)| BNO085 VCC       | 3.3V    | Digital Logic Power            |
| Pico 3V3 (OUT)| Encoder L/R VCC  | 3.3V    | DO NOT use 5V for Encoders!    |
| Pico GND      | All Modules GND  | 0V      | Common Ground Requirement      |

## 3. PIO Configuration
* **Program**: `quadrature_encoder.pio`
* **Base Pin L**: GP12 (Reads 12 & 13)
* **Base Pin R**: GP14 (Reads 14 & 15)