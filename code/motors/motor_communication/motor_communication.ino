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

#define RAMP_MAX 255
#define MOTOR_TIME 5000 // ms

const int motors[4][3] = {
  {10, 9, 8},    // Motor 1
  {11, 12, 13},  // Motor 2
  {4, 5, 6},     // Motor 3
  {7, 2, 3}      // Motor 4
};

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

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

/*void motor_jog_one(int max, int min, int p_pin, int dir, int d_pin_1, int d_pin_2, int time) {

  // limit max, min & time
  if (max > RAMP_MAX) { max = RAMP_MAX; }
  if (max < 0) { max = 0; }
  if (min > max) { min = max; }
  if (min < 0) { min = 0; }
  if (time > MOTOR_TIME) { time = MOTOR_TIME; }
  if (time < 0) { time = 0; }

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
  for (int duty = max; duty >= 0; duty--) {
    analogWrite(p_pin, duty);
    delay(1);
  }
}*/

void dir_move_two(int max, int min, int dir ,int time) {

  // limit max, min & time
  if (max > RAMP_MAX) { max = RAMP_MAX; }
  if (max < 0) { max = 0; }
  if (min > max) { min = max; }
  if (min < 0) { min = 0; }
  if (time > MOTOR_TIME) { time = MOTOR_TIME; }
  if (time < 0) { time = 0; }

  int m1 = 0;
  int m2 = 0;
  if (dir == 45) {
    m1 = 1;
    m2 = 3;
    set_dir_motor(0, motors[1][0], motors[1][1]);
    set_dir_motor(0, motors[3][0], motors[3][1]);
  } else if (dir == 135) {
    m1 = 0;
    m2 = 2;
    set_dir_motor(1, motors[0][0], motors[0][1]);
    set_dir_motor(1, motors[2][0], motors[2][1]);
  } else if (dir == 225) {
    m1 = 1;
    m2 = 3;
    set_dir_motor(1, motors[1][0], motors[1][1]);
    set_dir_motor(1, motors[3][0], motors[3][1]);
  } else if (dir == 315) {
    m1 = 0;
    m2 = 2;
    set_dir_motor(0, motors[0][0], motors[0][1]);
    set_dir_motor(0, motors[2][0], motors[2][1]);
  }
  for (int duty = min; duty <= max; duty++) {
    analogWrite(motors[m1][2], duty);
    analogWrite(motors[m2][2], duty);
    delay(1);
  }
  delay(time);
  for (int duty = max; duty >= 0; duty--) {
    analogWrite(motors[m1][2], duty);
    analogWrite(motors[m2][2], duty);
    delay(1);
  }
}

void dir_move_all(int max, int min, int dir,int time) {

  // limit max, min & time
  if (max > RAMP_MAX) { max = RAMP_MAX; }
  if (max < 0) { max = 0; }
  if (min > max) { min = max; }
  if (min < 0) { min = 0; }
  if (time > MOTOR_TIME) { time = MOTOR_TIME; }
  if (time < 0) { time = 0; }

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
  for (int duty = max; duty >= 0; duty--) {
    for (int m = 0; m < 4; m++) {
      analogWrite(motors[m][2], duty);
    }
    delay(1);
  }
}

void turn_all(int max, int min, int dir,int time) {

  // limit max, min & time
  if (max > RAMP_MAX) { max = RAMP_MAX; }
  if (max < 0) { max = 0; }
  if (min > max) { min = max; }
  if (min < 0) { min = 0; }
  if (time > MOTOR_TIME) { time = MOTOR_TIME; }
  if (time < 0) { time = 0; }

  // 1 = left
  // 0 = right
  if (dir == 1) {
    set_dir_motor(0, motors[0][0], motors[0][1]);
    set_dir_motor(1, motors[1][0], motors[1][1]);
    set_dir_motor(1, motors[2][0], motors[2][1]);
    set_dir_motor(0, motors[3][0], motors[3][1]);
  } else if (dir ==  0) {
    set_dir_motor(1, motors[0][0], motors[0][1]);
    set_dir_motor(0, motors[1][0], motors[1][1]);
    set_dir_motor(0, motors[2][0], motors[2][1]);
    set_dir_motor(1, motors[3][0], motors[3][1]);
  }
  for (int duty = min; duty <= max; duty++) {
    for (int m = 0; m < 4; m++) {
      analogWrite(motors[m][2], duty);
    }
    delay(1);
  }
  delay(time);
  // ---- RAMP DOWN ----
  for (int duty = max; duty >= 0; duty--) {
    for (int m = 0; m < 4; m++) {
      analogWrite(motors[m][2], duty);
    }
    delay(1);
  }
}

void setup() {
  Serial.begin(9600);
  for (int m = 0; m < 4; m++) {
    for (int p = 0; p < 3; p++) {
      pinMode(motors[m][p], OUTPUT);
    }
  }
}

void loop() {
  if (Serial.available()) {
    String command_raw = Serial.readStringUntil('\n');
    String command = getValue(command_raw, ':', 0);
    if (command == "dir_move_all") {
      int arg1 = getValue(command_raw, ':', 1).toInt();
      int arg2 = getValue(command_raw, ':', 2).toInt();
      int arg3 = getValue(command_raw, ':', 3).toInt();
      int arg4 = getValue(command_raw, ':', 4).toInt();
      Serial.printf("cmd %s, arg1 %d, arg2 %d, arg3 %d, arg4 %d\n", command, arg1, arg2, arg3, arg4);
      dir_move_all(arg1, arg2, arg3, arg4);
    } else if (command == "dir_move_two") {
      int arg1 = getValue(command_raw, ':', 1).toInt();
      int arg2 = getValue(command_raw, ':', 2).toInt();
      int arg3 = getValue(command_raw, ':', 3).toInt();
      int arg4 = getValue(command_raw, ':', 4).toInt();
      Serial.printf("cmd %s, arg1 %d, arg2 %d, arg3 %d, arg4 %d\n", command, arg1, arg2, arg3, arg4);
      dir_move_two(arg1, arg2, arg3, arg4);
    } else if (command == "turn_all") {
      int arg1 = getValue(command_raw, ':', 1).toInt();
      int arg2 = getValue(command_raw, ':', 2).toInt();
      int arg3 = getValue(command_raw, ':', 3).toInt();
      int arg4 = getValue(command_raw, ':', 4).toInt();
      Serial.printf("cmd %s, arg1 %d, arg2 %d, arg3 %d, arg4 %d\n", command, arg1, arg2, arg3, arg4);
      turn_all(arg1, arg2, arg3, arg4);
    }
  }
  delay(1);
}

