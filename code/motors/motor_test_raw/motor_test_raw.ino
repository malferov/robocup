// const int pwmPinA_1 = 3;  
// const int pwmPinB_1 = 6;
// const int pinA_1_1 = 7;  
// const int pinA_2_1 = 2;  
// const int pinB_1_1 = 4;
// const int pinB_2_1 = 5;
// const int pwmPinA_2 = 13;  
// const int pwmPinB_2 = 8;
// const int pinA_1_2 = 12;  
// const int pinA_2_2 = 11;  
// const int pinB_1_2 = 10;
// const int pinB_2_2 = 9;
// motors[motor][pin]
// pin 0 = DIR1
// pin 1 = DIR2
// pin 2 = PWM

const int motors[4][3] = {
  {7, 2, 3},    // Motor 1
  {4, 5, 6},    // Motor 2
  {11, 12, 13}, // Motor 3
  {10, 9, 8}    // Motor 4
};


// 1 = 1,0
//0 = 0,1
void set_dir_motor(int buul,int pin_1, int pin_2) {
  if (buul == 1) {
    digitalWrite(pin_1, 1);
    digitalWrite(pin_2, 0);
  } else {
    digitalWrite(pin_1, 0);
    digitalWrite(pin_2, 1);
  }
}

void motor_jog_one(int max,int min, int p_pin, int dir , int d_pin_1, int d_pin_2, int time) 
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
    delay(1);
  }
  delay(time);
  for (int duty = max; duty >= min; duty--) {
    analogWrite(p_pin, duty);
    delay(1);
  }
}
void motor_jog_double(int max, int min, int dir, int m1, int m2,int time) {
  set_dir_motor(dir, motors[m1][0], motors[m1][1]);
  set_dir_motor(dir, motors[m2][0], motors[m2][1]);

  for (int duty = min; duty <= max; duty++) {
    analogWrite(motors[m1][2], duty);
    analogWrite(motors[m2][2], duty);
    delay(1);
  }
  delay(time);
  for (int duty = max; duty >= min; duty--) {
    analogWrite(motors[m1][2], duty);
    analogWrite(motors[m2][2], duty);
    delay(1);
  }
}
void all_motor_jog(int max, int min, int dir,int time) {

  // ---- SET DIRECTION ----
  if (dir == 0) {                 // FORWARD
    for (int m = 0; m < 4; m++) {
      set_dir_motor(0, motors[m][0], motors[m][1]);
    }

  } else if (dir == 180) {        // BACKWARD
    for (int m = 0; m < 4; m++) {
      set_dir_motor(1, motors[m][0], motors[m][1]);
    }

  } else if (dir == 90) {         // ROTATE RIGHT
    // left side forward
    set_dir_motor(1, motors[0][0], motors[0][1]);
    set_dir_motor(1, motors[2][0], motors[2][1]);

    // right side backward
    set_dir_motor(0, motors[1][0], motors[1][1]);
    set_dir_motor(0, motors[3][0], motors[3][1]);

  } else if (dir == 270) {        // ROTATE LEFT
    // left side backward
    set_dir_motor(0, motors[0][0], motors[0][1]);
    set_dir_motor(0, motors[2][0], motors[2][1]);

    // right side forward
    set_dir_motor(1, motors[1][0], motors[1][1]);
    set_dir_motor(1, motors[3][0], motors[3][1]);

  } else {
    return; // invalid direction
  }

  // ---- RAMP UP ----
  for (int duty = min; duty <= max; duty++) {
    for (int m = 0; m < 4; m++) {
      analogWrite(motors[m][2], duty);
    }
    delay(1);
  }
  delay(time);
  // ---- RAMP DOWN ----
  for (int duty = max; duty >= min; duty--) {
    for (int m = 0; m < 4; m++) {
      analogWrite(motors[m][2], duty);
    }
    delay(1);
  }
}

void setup() {
  for (int m = 0; m < 4; m++) {
    for (int p = 0; p < 3; p++) {
      pinMode(motors[m][p], OUTPUT);
    }
  }
}


void loop() {
  all_motor_jog(155,0,0,500);
  delay(300);
  all_motor_jog(155,0,180,500);
  
  delay(300);
  
  motor_jog_double(155,0,0,1,3,500);
  delay(300);
  motor_jog_double(155,0,1,1,3,500);

  delay(300);

  all_motor_jog(155,0,90,500);
  delay(300);
  all_motor_jog(155,0,270,500);
  
  delay(300);

  motor_jog_double(155,0,1,0,2,500);
  delay(300);
  motor_jog_double(155,0,0,0,2,500);

  delay(300);

  all_motor_jog(155,0,180,500);
  delay(300);
  all_motor_jog(155,0,0,500);
  
  delay(300);
  
  motor_jog_double(155,0,1,1,3,500);
  delay(300);
  motor_jog_double(155,0,0,1,3,500);

  delay(300);

  all_motor_jog(155,0,270,500);
  delay(300);
  all_motor_jog(155,0,90,500);
  
  delay(300);

  motor_jog_double(155,0,0,0,2,500);
  delay(300);
  motor_jog_double(155,0,1,0,2,500);

  delay(300);
}

