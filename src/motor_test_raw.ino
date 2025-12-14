const int pwmPinA_1 = 3;  
const int pwmPinB_1 = 6;
const int pinA_1_1 = 7;  
const int pinB_1_1 = 4;
const int pinA_2_1 = 2;  
const int pinB_2_1 = 5;
const int pwmPinA_2 = 13;  
const int pwmPinB_2 = 8;
const int pinA_1_2 = 12;  
const int pinB_1_2 = 10;
const int pinA_2_2 = 11;  
const int pinB_2_2 = 9;


void motor_jog_one(int max,int min, int p_pin, int dir , int d_pin_1, int d_pin_2) 
{
  if (dir == 1) {
    digitalWrite(d_pin_1, 1);
    digitalWrite(d_pin_2, 0);
  } else {
    digitalWrite(d_pin_1, 0);
    digitalWrite(d_pin_2, 1);
  }
  for (int duty = min; duty <= max; duty++) {
    analogWrite(p_pin, duty);
    delay(5);
  }
  for (int duty = max; duty >= min; duty--) {
    analogWrite(p_pin, duty);
    delay(5);
  }
}
void motor_jog_double(int max,int min, int p_pin_1, int p_pin_2, int dir , int d_pin_1_1, int d_pin_2_1,int d_pin_1_2, int d_pin_2_2) 
{
  if (dir == 1) {
    digitalWrite(d_pin_1_1, 1);
    digitalWrite(d_pin_2_1, 0);
    digitalWrite(d_pin_1_2, 1);
    digitalWrite(d_pin_2_2, 0);
  } else {
    digitalWrite(d_pin_1_1, 0);
    digitalWrite(d_pin_2_1, 1);
    digitalWrite(d_pin_1_2, 0);
    digitalWrite(d_pin_2_2, 1);
  }
  for (int duty = min; duty <= max; duty++) {
    analogWrite(p_pin_1, duty);
    analogWrite(p_pin_2, duty);
    delay(5);
  }
  for (int duty = max; duty >= min; duty--) {
    analogWrite(p_pin_1, duty);
    analogWrite(p_pin_2, duty);
    delay(5);
  }
}

void setup() {
  pinMode(pinA_1_1, OUTPUT);
  pinMode(pinA_2_1, OUTPUT);
  pinMode(pwmPinA_1, OUTPUT);

  pinMode(pinB_1_1, OUTPUT); 
  pinMode(pinB_2_1, OUTPUT);
  pinMode(pwmPinB_1, OUTPUT);

  pinMode(pinA_1_2, OUTPUT);
  pinMode(pinA_2_2, OUTPUT);
  pinMode(pwmPinA_2, OUTPUT);

  pinMode(pinB_1_2, OUTPUT); 
  pinMode(pinB_2_2, OUTPUT);
  pinMode(pwmPinB_2, OUTPUT);
}

void loop() {
  motor_jog_double(255,0,pwmPinB_1,pwmPinB_2,1,pinB_1_1,pinB_2_1,pinB_1_2,pinB_2_2);
  delay(300);
  motor_jog_double(255,0,pwmPinB_1,pwmPinB_2,0,pinB_1_1,pinB_2_1,pinB_1_2,pinB_2_2);

  delay(300);

  motor_jog_double(255,0,pwmPinA_1,pwmPinA_2,1,pinA_1_1,pinA_2_1,pinA_1_2,pinA_2_2);
  delay(300);
  motor_jog_double(255,0,pwmPinA_1,pwmPinA_2,0,pinA_1_1,pinA_2_1,pinA_1_2,pinA_2_2);
  delay(300);
}

