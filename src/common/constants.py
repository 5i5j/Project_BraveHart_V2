# src/common/constants.py

# Robot metadata
ROBOT_ID = "picar_x_001"
VERSION = "2.0.0"

# frequency configuration (Pico will output data at this frequency)
IMU_HZ = 50
ENCODER_HZ = 50
POWER_HZ = 10  # INA260 power sensor output frequency

# Serial communication settings
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 115200
TIMEOUT = 0.1  # non-blocking read

# Data storage and S3 configuration
DATA_DIR = "./data/raw"
S3_BUCKET = "edge-to-cloud-robotics-landing-s3"