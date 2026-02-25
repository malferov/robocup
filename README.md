# RoboCup
RoboCup Junior Netherlands team `CoreX`

<p align="center">
    <img src="corex_logo.png" alt="CoreX" width="300">
</p>

Test

![image](corex_logo.png){height=300}


# Components
- Motors: 25mm DC 12V GA-370 Low Speed Metal Gear Motor 400RPM 3.5 Watts 90 g
- Motor's driver: TB6612FNG Dual Driver Controller Board MOSFET H Bridge 15V max VCC=2.7-5.5V Output 1.2A/3.2A
- Battery: 12.6V 3000mAh Lithium Ion Battery 157 g
- Power: OT253-B47 Step-down buck converter 4.5-24V to 5V 3A
- Distance sensor: HC-SR04 Ultrasonic Sensor
- Line detector: TCRT5000 Reflective Infrared Optical Sensor
- IR sensor: Vishay TSSP4038, 38kHz IR Receiver, 950nm ±45°, 25m Range, 2.5-5.5V
- MCU: ESP32-S3

# Metrics
### Consumption
- Main + CAM + Driver: 170 mA
### Performance
- Distance sensor: 33 ms
- Camera capture: 34 ms
- get_pos: 98 ms
