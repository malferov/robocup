#define RAMP_MAX 255
#define RAMP_STEP 1
#define MOTOR_TIME 15000 // ms

// motors[motor][pin]
// pin 0 = DIR1
// pin 1 = DIR2
// pin 2 = PWM

#define BOT_ID 1 // 1 or 2

#if BOT_ID == 1
const int motors[4][3] = {
  {9, 10, 8},    // Motor RF1
  {2, 7, 3},     // Motor RB2
  {5, 4, 6},     // Motor LB3
  {12, 11, 13}   // Motor LF4
};
#else
const int motors[4][3] = {
  {2, 3, 1},     // Motor RF1 2A
  {12, 13, 11},  // Motor RB2 1A
  {8, 9, 10},    // Motor LB3 1B
  {4, 5, 6}      // Motor LF4 2B
};
#endif

int targetRF1 = 0;
int targetRB2 = 0;
int targetLB3 = 0;
int targetLF4 = 0;

int currentRF1 = 0;
int currentRB2 = 0;
int currentLB3 = 0;
int currentLF4 = 0;

String getValue(String data, char separator, int index) {
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

// 1 = 1,0 forward
// 0 = 0,1 reverse
void set_dir_motor(int dir, int pin_1, int pin_2) {
  if (dir == 1) {
    digitalWrite(pin_1, 1);
    digitalWrite(pin_2, 0);
  } else {
    digitalWrite(pin_1, 0);
    digitalWrite(pin_2, 1);
  }
}

void dir_move_all(int max, int min, int dir, int time) {

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
      set_dir_motor(1, motors[m][0], motors[m][1]);
    }

  } else if (dir == 180) {        // BACKWARD
    for (int m = 0; m < 4; m++) {
      set_dir_motor(0, motors[m][0], motors[m][1]);
    }

  } else if (dir == 90) {         // SLIDE RIGHT
    set_dir_motor(0, motors[0][0], motors[0][1]);
    set_dir_motor(1, motors[1][0], motors[1][1]);
    set_dir_motor(0, motors[2][0], motors[2][1]);
    set_dir_motor(1, motors[3][0], motors[3][1]);

  } else if (dir == 270) {        // SLIDE LEFT
    set_dir_motor(1, motors[0][0], motors[0][1]);
    set_dir_motor(0, motors[1][0], motors[1][1]);
    set_dir_motor(1, motors[2][0], motors[2][1]);
    set_dir_motor(0, motors[3][0], motors[3][1]);

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

void speed4(int FL, int FR, int RR, int RL, int time) {

  // limit speed
  int speed[4] = {FL, FR, RR, RL};
  for (int m = 0; m < 4; m++) {
    if (speed[m] > RAMP_MAX) { speed[m] = RAMP_MAX; }
    if (speed[m] < -RAMP_MAX) { speed[m] = -RAMP_MAX; }
  }
  // limit time
  if (time > MOTOR_TIME) { time = MOTOR_TIME; }
  if (time < 0) { time = 0; }
  // set direction
  for (int m = 0; m < 4; m++) {
    if (speed[m] > 0) {
      set_dir_motor(1, motors[m][0], motors[m][1]);
    } else {
      set_dir_motor(0, motors[m][0], motors[m][1]);
    }
    // clear direction (for ramping)
    speed[m] = abs(speed[m]);
  }
  // ---- RAMP UP ----
  for (int duty = 0; duty <= RAMP_MAX; duty++) {
    bool done[4] = {false, false, false, false};
    for (int m = 0; m < 4; m++) {
      if (speed[m] >= duty) {
        analogWrite(motors[m][2], duty);
      } else {
        done[m] = true;
      }
    }
    delay(1);
    if (done[0] && done[1] && done[2] && done[3]) {
      break;
    }
  }
  // ---- RUN ----
  delay(time);
  // ---- RAMP DOWN ----
  for (int duty = 0; duty <= RAMP_MAX; duty++) {
    bool done[4] = {false, false, false, false};
    for (int m = 0; m < 4; m++) {
      if (speed[m] >= duty) {
        analogWrite(motors[m][2], speed[m] - duty); // invert
      } else {
        done[m] = true;
      }
    }
    delay(1);
    if (done[0] && done[1] && done[2] && done[3]) {
      break;
    }
  }
}

void rampMotor(int motor, int *current, int target) {
    int m = motor-1;
    // ramp current
    if (*current < target) {
        *current += RAMP_STEP;
        if (*current > target)
            *current = target;
    }
    else if (*current > target) {
        *current -= RAMP_STEP;
        if (*current < target)
            *current = target;
    }
    // set direction
    if (abs(*current) <= RAMP_STEP) {
        if (*current > 0) {
            set_dir_motor(1, motors[m][0], motors[m][1]);
        } else {
            set_dir_motor(0, motors[m][0], motors[m][1]);
        }
    }
    // set pwm
    analogWrite(motors[m][2], abs(*current));
}

void stop() {
  // set target zero
  targetRF1 = 0;
  targetRB2 = 0;
  targetLB3 = 0;
  targetLF4 = 0;
  // define max pwm
  int max = 0;
  if (abs(currentRF1) > max) max = abs(currentRF1);
  if (abs(currentRB2) > max) max = abs(currentRB2);
  if (abs(currentLB3) > max) max = abs(currentLB3);
  if (abs(currentLF4) > max) max = abs(currentLF4);
  // ramp down
  for (int duty = 0; duty <= max; duty++) {
    rampMotor(1, &currentRF1, targetRF1);
    rampMotor(2, &currentRB2, targetRB2);
    rampMotor(3, &currentLB3, targetLB3);
    rampMotor(4, &currentLF4, targetLF4);
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
      stop();
      dir_move_all(arg1, arg2, arg3, arg4);
    } else if (command == "speed4") {
      int arg1 = getValue(command_raw, ':', 1).toInt();
      int arg2 = getValue(command_raw, ':', 2).toInt();
      int arg3 = getValue(command_raw, ':', 3).toInt();
      int arg4 = getValue(command_raw, ':', 4).toInt();
      int arg5 = getValue(command_raw, ':', 5).toInt();
      stop();
      speed4(arg1, arg2, arg3, arg4, arg5);
    } else if (command == "target") {
      targetRF1 = getValue(command_raw, ':', 1).toInt();
      targetRB2 = getValue(command_raw, ':', 2).toInt();
      targetLB3 = getValue(command_raw, ':', 3).toInt();
      targetLF4 = getValue(command_raw, ':', 4).toInt();
      // debug
      //Serial.printf("cmd %s, arg1 %d, arg2 %d, arg3 %d, arg4 %d\n", command, targetRF1, targetRB2, targetLB3, targetLF4);
    }
  }

  rampMotor(1, &currentRF1, targetRF1);
  rampMotor(2, &currentRB2, targetRB2);
  rampMotor(3, &currentLB3, targetLB3);
  rampMotor(4, &currentLF4, targetLF4);

  delay(1);
}

