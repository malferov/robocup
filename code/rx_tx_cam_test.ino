void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() {
  Serial2.write("get_pos\n");
  if (Serial2.available()) {
    Serial.println(Serial2.readStringUntil('\n'));
  }
  delay(1000);
}
