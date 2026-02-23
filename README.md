# Project BraveHart V2

An integrated robotics framework for high-frequency Ego Dynamics and Perception.

## Project Structure
- `src/common`: Shared constants and utility functions.
- `src/drivers`: Hardware abstraction layer (HAL) for sensors.
- `src/nodes`: Functional computation units (Nodes).
- `src/firmware`: Embedded code for microcontrollers (Pico/STM32).

## Hardware Targets
- **Pi 3B**: Runs `nodes/ego_dynamics/collector.py` (50Hz Data Logging).
- **P620 Server**: Runs `nodes/ego_dynamics/analyzer.py` (Kinematics Analysis).
- **Jetson Orin**: Future target for `nodes/perception_node`.

## Setup
1. Run `./scripts/setup_env.sh` to initialize venv.
2. Deploy code using `./scripts/deploy.sh <target_ip>`.