# WheelBot.org Controller Module

## Description
The WheelBot Controller Module project is firmware for a  WheelBot based on the ESP8266 microcontroller. It is designed for autonomous or remote control of the robot via MQTT.

Wheelbot.org is a $30 wheeled robot with an open architecture for research into machine learning algorithms and autonomous systems.

### Features
- **Sensor Suite**: Ultrasonic sensors (HC-SR04) for obstacle detection, IMU (MPU6050) for orientation and motion tracking, power monitoring (INA226).
- **Actuators**: Two DC motors with PWM for speed control and servo for steering.
- **Connectivity**: MQTT over Wi-Fi for telemetry and commands.
- **Setup**: Easy WiFi configuration via captive portal.

## Installation
1. Install [PlatformIO](https://platformio.org/) (or VS Code with PlatformIO extension).
2. Clone the repository `git clone https:\\github.com\wheelbot-org\wheelbot-ctrl.git`.
3. Build the project `cd wheelbot-ctrl && pio run`.
4. Flash to ESP8266: Connect via USB, run `pio run -t erase && pio run -t upload && pio run -t uploadfs`.

### Requirements
- ESP8266 NodeMCU v2 board.
- Sensors: HC-SR04 (x2), MPU6050, INA226.
- Actuators: DC motors (x2), servo motor.
- Libraries: Auto-installed via PlatformIO (see `platformio.ini`).

## Usage
1. Power on; if no WiFi config, connect to "Wheelbot-Ctrl-Setup" portal and set SSID/password.
2. Robot connects to WiFi/MQTT and publishes sensor data (e.g., `/sensors/json`).
3. Send MQTT commands to control motors/steering (e.g., JSON payloads for speed/direction).
4. Monitor via serial logs (115200 baud) or MQTT subscriptions.

### Example MQTT Payloads
- **Sensor Data**: `{"sonars":{"left":50,"right":60},"accelerometr":{"x":0.1,"y":0,"z":9.8},...}`
- **Control Data**: `{"engines":{"left":{"speed":100},"right":{"speed":100}},"steering":{"direction":45}}`

## MQTT Commands
| Command Name | Topic | Payload | Description |
|--------------|-------|---------|-------------|
| IMU Calibration | `service/calibrate-mcu` | Ignored | Starts MPU6050 calibration, publishes offsets to `service/calibrate-mcu-result`. |
| Restart | `service/restart` | Ignored | Restarts the device. |
| Left Engine Speed | `engines/left/speed_percent` | `int (-100..100)` | Sets left motor speed in percent (negative — backward). |
| Right Engine Speed | `engines/right/speed_percent` | `int (-100..100)` | Sets right motor speed in percent. |
| Left Engine Acceleration | `engines/left/acceleration` | `int` | Sets left motor acceleration. |
| Right Engine Acceleration | `engines/right/acceleration` | `int` | Sets right motor acceleration. |
| Steering Rotate | `steering-wheel/rotate` | `int (0..180)` | Sets steering angle in degrees. |
| Steering Acceleration | `steering-wheel/acceleration` | `int` | Sets steering acceleration. |

## Development
- Monitoring: `pio device monitor` for serial output.
- Debug: Enable `#define ENABLE_DEBUG` in `config.h`.
- Lint: Run `pio check` for static analysis.

## Contributing
Contributions welcome! Fork, make changes, and submit a merge request.

## Authors and Acknowledgments
- Developed by the WheelBot.org team.
- Thanks to open-source libs (NewPing, EspMQTTClient).

## License
Dual-licensed:
- [**Non-Commercial License**](./LICENSE-NC): Free for personal, educational, and research use.
- [**Commercial License**](./LICENSE-COMMERCIAL): Required for any commercial use. Contact: <iliya@bykonya.com>