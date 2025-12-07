const int pwmPinA = 3;  
const int pwmPinB = 6;



void setup() {
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(pwmPinA, OUTPUT);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(pwmPinB, OUTPUT);
}

void loop() {

  digitalWrite(1, 1);
  digitalWrite(2, 0);
  
  digitalWrite(4, 1);
  digitalWrite(5, 0);

  // analogWrite(pwmPinA, 255);

  for (int duty = 0; duty <= 255; duty++) {
    analogWrite(pwmPinA, duty);
    analogWrite(pwmPinB, duty);
    delay(5);
  }
  for (int duty = 255; duty >= 0; duty--) {
    analogWrite(pwmPinA, duty);
    analogWrite(pwmPinB, duty);
    delay(5);
  }

  digitalWrite(1, 0);
  digitalWrite(2, 1);

  digitalWrite(4, 0);
  digitalWrite(5, 1);

  for (int duty = 0; duty <= 255; duty++) {
    analogWrite(pwmPinA, duty);
    analogWrite(pwmPinB, duty);
    delay(5);
  }
  for (int duty = 255; duty >= 0; duty--) {
    analogWrite(pwmPinA, duty);
    analogWrite(pwmPinB, duty);
    delay(5);
  }
}

