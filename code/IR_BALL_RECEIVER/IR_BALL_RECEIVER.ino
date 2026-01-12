#define RECEIVER_PIN 4

void setup() {
  Serial.begin(9600);
}

void loop() {
  int val = analogRead(RECEIVER_PIN);
  int valx = (4095 - val)/4095.0*100;
  Serial.println(valx);
  delay(200);
}
