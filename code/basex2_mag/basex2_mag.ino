#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

// I2C Address 0x29
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);

float targetAngle = -1.0; 

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Robot Compass - Simplified Setup");

  // Custom Pins for ESP32-C3: SDA (12) and SCL (14)
  Wire.begin(12, 14); 
  Wire.setClock(100000); 

  // .begin() handles the Mode setting for you!
  if (!bno.begin()) {
    Serial.println("BNO055 not found on Pins 12/14! Check wires.");
    while (1);
  }

  // Use internal oscillator for stability
  bno.setExtCrystalUse(false);
  
  delay(1000);
  Serial.println("Setup Complete. Keep robot still to calibrate Gyro.");
}

void loop() {
  sensors_event_t event;
  bno.getEvent(&event);

  float currentAngle = event.orientation.x; // Heading 0-360

  uint8_t system, gyro, accel, mag;
  bno.getCalibration(&system, &gyro, &accel, &mag);

  // Set target once Gyro is calibrated (G > 0)
  if (targetAngle < 0 && gyro > 0) {
    targetAngle = currentAngle;
    Serial.print(">>> TARGET LOCKED AT: ");
    Serial.println(targetAngle);
  }

  if (targetAngle >= 0) {
    float offset = currentAngle - targetAngle;

    // Correct for the 360/0 degree wrap-around
    // This ensures -5 degrees is "Turn Left" instead of +355 degrees
    if (offset > 180)  offset -= 360;
    if (offset < -180) offset += 360;

    Serial.print("Heading: "); Serial.print(currentAngle);
    Serial.print(" | OFFSET: "); Serial.print(offset);
    
    // DEADZONE: If offset is less than 2 degrees, consider it "Straight"
    if (abs(offset) < 2.0) {
      Serial.println(" [ON TRACK]");
    } else {
      // Positive offset = Robot turned Right (needs to turn Left)
      // Negative offset = Robot turned Left (needs to turn Right)
      Serial.println(offset > 0 ? " [TURN LEFT]" : " [TURN RIGHT]");
    }
  } else {
    Serial.print("Waiting for Gyro... Current G-Status: "); 
    Serial.println(gyro);
  }

  delay(100);
}
