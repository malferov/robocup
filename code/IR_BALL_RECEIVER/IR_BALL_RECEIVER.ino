#define RECEIVER_PIN 3
#define RECEIVER_ADDRESS 4
#define ANALOG_MAX 4095

void setup() {
  Serial.begin(9600);
  pinMode(RECEIVER_ADDRESS, OUTPUT);  // set the digital pin RECEIVER_ADDRESS as output
}

int getIR(int channel) {
  digitalWrite(RECEIVER_ADDRESS, channel);   // select channel
  int val = analogRead(RECEIVER_PIN);
  //float valf = (ANALOG_MAX - val) / ANALOG_MAX * 100;   // normalization
  return val;
}

int getPosition() {
  int channel = -1;   // unknown position
  int min = ANALOG_MAX;
  for (int i = 0; i < 2; i++) {
    int ir = getIR(i);
    if (ir < min) {
      min = ir;
      channel = i;
    }
  }

  if (channel == -1) {
    return -1;   // can't detect
  }

  if (min > 0.5 * ANALOG_MAX) {
    return -1;   // filter noise
  }

  return 90 + channel * 180;
}

void loop() {
    Serial.println(getPosition());
    delay(200);
}
