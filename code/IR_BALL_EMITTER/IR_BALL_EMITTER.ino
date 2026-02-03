#define PWM_PIN 3

void setup() {
  // Set PWM frequency
  analogWriteFrequency(PWM_PIN, 40000);  // 40 kHz

  // Set PWM resolution (bits)
  analogWriteResolution(PWM_PIN, 8);     // 0–255

  // 50% duty cycle
  analogWrite(PWM_PIN, 128);
}

void loop() {
  // PWM runs in hardware
  delay(1);
}
