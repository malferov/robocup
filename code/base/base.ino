#include <Wire.h>
#include <Adafruit_GFX.h>
#include "DFRobot_BMM150.h"
#include <Preferences.h>
DFRobot_BMM150_I2C bmm150(&Wire1, 0x13);
Preferences prefs;

#define BOT_ID 2 // 1 or 2

#if BOT_ID == 1
#include <Adafruit_SSD1306.h>
#define BOT_ID 1
#define MUX_SIG 15
#define MUX_S3 2
#define MUX_S2 4
#define MUX_S1 5
#define MUX_S0 18
#define MODE_BUTTON 19
#define START_BUTTON 23
#define INIT_DISTANCE 0
#define LINE_LEFT 34
#define LINE_CENTER 36
#define LINE_RIGHT 39
#else
#include <Adafruit_SH110X.h>
#define MUX_SIG 13
#define MUX_S3 15
#define MUX_S2 2
#define MUX_S1 0
#define MUX_S0 4
#define MODE_BUTTON 26
#define START_BUTTON 23
#define INIT_DISTANCE 200 // fixed distance for bot2
#define LINE_LEFT 32
#define LINE_CENTER 33
#define LINE_RIGHT 25
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 21
#define SCL_PIN 22

#define SERIAL_RX 3
#define SERIAL_TX 1
#define SERIAL2_RX 16
#define SERIAL2_TX 17

#define ANALOG_MAX 4095
#define LINE_THRESHOLD 4050
#define SEGMENT_ANGLE 24

#define PRESS_DELAY 500   // ms
#define SHOOT_DELAY 1000  // ms
#define EXTRA_CORR 0

#define ACPT_DEVIATION 8 //acceptable deviation angle
#define ACPT_DISTANCE 50 // mm
#define ONE_SPEED 15
#define MAX_BALL_DEVIATION 10 //degrees

# if BOT_ID == 1
#define MIN_SPEED 7
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#else
#define MIN_SPEED 15
#define WHITE SH110X_WHITE
#define BLACK SH110X_BLACK
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

typedef struct {
  char dir;
  int deviation;
  int left;
  int right;
  int center;
} goalPositionT;

typedef struct {
  goalPositionT goalPosition;
  int distance;
} CAMT;

goalPositionT zeroPosition = goalPositionT('L',0,0,0,0);
CAMT cam = CAMT(zeroPosition, INIT_DISTANCE);
int ballHeading = 0;  // ahead by default
int ballDeviation = 0;
int accept_distance = ACPT_DISTANCE;
int angs[15];         // sensors angles

String Modes[] = {
  "SEARCH_MAG",
  "CHASE_HALF",
  "GOAL_SEARCH",
  "GOAL_KEEPER",
  "BALL_CHASE",
  "BALL_HOOK",
  "BALL_SEARCH",
  "CALIBRATION",
  "LINE_ACTIVE",
  "SPEED_1",
  "SPEED_2",
  "SPEED_3"
};
int mode_len = 12;
int mode_num = 0;
bool idle = true;
String mode = Modes[mode_num];
int max_speed = MIN_SPEED + ONE_SPEED * 1; // 1, 2 or 3

unsigned long button_timer;
unsigned long move_timer;
unsigned long shoot_timer;
unsigned long distance_reset_timer;
bool need_reset = false;
char msg[50] = {0};

#define CALIBRATION_TIME 15000
float mag_x_offset = 0;
float mag_y_offset = 0;
int magHeading = 0;
int magChase = 0;
int magDeviation = 0;
bool line_active = false;
bool line_right = false;
bool line_left = false;

// debug
unsigned long debug_timer;
int debug_dir = 0;
int debug_num = 0;

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

int getIR(int channel) {
  // select channel
  digitalWrite(MUX_S3, (channel >> 3) & 0x1);
  digitalWrite(MUX_S2, (channel >> 2) & 0x1);
  digitalWrite(MUX_S1, (channel >> 1) & 0x1);
  digitalWrite(MUX_S0, channel & 0x1);
  // read value
  int val = analogRead(MUX_SIG);
  // invert
  val = ANALOG_MAX - val;
  // normalization
  //val = (ANALOG_MAX - val) / ANALOG_MAX * 100;
  // noise
  // ANALOG_MAX * 0.01 = 40
  if (val >= 40) {
    return 1;
  }
  return 0;
}

void calcBallHeading() {

  int angle = ballHeading;
  int irs[15];
  int sum = 0;
  int cnt = 0;
  // get current angle
  // see ChatGPT Circular Mean (Vector Averaging) for trig solution
  // and Mean Delta lightweight option when all angles lie on the same "side" of the circle
  for (int i = 0; i < 15; i++) {
    irs[i] = getIR(i);
    if (irs[i] == 1) {
      cnt++;
      int delta = angs[i] - angle;
      if (delta > 180) {
        delta -= 360;
      }
      if (delta < -180) {
        delta += 360;
      }
      sum = sum + delta;
    }
  }
  if (cnt > 0) {
    // low-pass filter 10%
    angle += (sum / cnt) * 0.1;
  }
  // wrap to 0–360
  if (angle < 0) {
    angle += 360;
  }
  if (angle >= 360) {
    angle -= 360;
  }
  ballHeading = angle;
}

void getCAM() {
  String command = Serial2.readStringUntil('\n');
  if (command[0] == 'L' || command[0] == 'R') {
    char dir = getValue(command,':',0)[0];
    int deviation = getValue(command,':',1).toInt();
    int left = getValue(command,':',2).toInt();
    int right = getValue(command,':',3).toInt();
    int center = getValue(command,':',4).toInt();
    cam.goalPosition = goalPositionT(dir, deviation, left, right, center);
    int distance = getValue(command,':',5).toInt();
    // filter spikes
    if (abs(distance - cam.distance) < 10) {
      // calculate average 10/90
      cam.distance = 0.1 * distance + 0.9 * cam.distance;
    } else {
      // reset timeout
      if (!need_reset) {
        need_reset = true;
        distance_reset_timer = millis() + 1000;
      }
      unsigned long current = millis();
      if (current > distance_reset_timer) {
        cam.distance = distance;
        need_reset = false;
      }
    }
  }
}

void refreshDisplay() {
  display.clearDisplay(); //clear display
  // display the data corresponding to the mode
  if (mode == "BALL_SEARCH") {
    // ball heading
    display.setCursor(0, 20);
    display.print(ballHeading);
    display.setCursor(0, 30);
    display.print(ballDeviation);
  } else if (mode == "DISTANCE_READ") {
    // distance
    display.setCursor(0, 20);
    display.printf("%03d", cam.distance);
  } else if (mode == "GOAL_SEARCH") {
    // camera
    display.setCursor(SCREEN_WIDTH-6*2, SCREEN_HEIGHT-8); // Size 1: 6x8 pixel area
    display.printf("%c%d", cam.goalPosition.dir, cam.goalPosition.deviation);
    int difference = cam.goalPosition.right - cam.goalPosition.left;
    display.drawRect((SCREEN_WIDTH - difference) / 2, (SCREEN_HEIGHT - 30) / 2, difference, 30, WHITE);
    display.fillRect(cam.goalPosition.center - 5, (SCREEN_HEIGHT - 50) / 2, 10, 50, WHITE);
  } else if (mode == "BALL_CHASE" || mode == "BALL_HOOK") {
    display.setCursor(0, 20);
    display.printf("heading  %3d", ballHeading);
    display.setCursor(0, 30);
    display.printf("distance %3d", cam.distance);
    display.setCursor(0, 40);
    display.printf("mag dev  %3d", magDeviation);
    display.setCursor(0, 50);
    display.printf("goal      %c%d", cam.goalPosition.dir, cam.goalPosition.deviation);
  } else if (mode == "IR_READ") {
    display.setCursor(0, 20);
    display.printf("heading %3d", ballHeading);
  } else if (mode == "GOAL_KEEPER") {
    if (ballHeading > MAX_BALL_DEVIATION && ballHeading < 180) {  
      display.setCursor(0, 20);
      display.print("R");
    } else if (ballHeading < 360-MAX_BALL_DEVIATION && ballHeading > 180) {
      display.setCursor(0, 20);
      display.print("L");
    } else {
      display.setCursor(0, 20);
      display.print("0");
    }
    display.setCursor(0, 40);
    display.print(ballHeading);
  } else if (mode == "CALIBRATION") {
    display.setCursor(0, 20);
    display.printf("mag heading %3d", magHeading);
  } else if (mode == "SEARCH_MAG") {
    display.setCursor(0, 20);
    display.printf("mag chase   %3d", magChase);
    display.setCursor(0, 30);
    display.printf("mag heading %3d", magHeading);
    display.setCursor(0, 40);
    display.printf("mag dev     %3d", magDeviation);
  } else if (mode == "CHASE_HALF") {
    display.setCursor(0, 20);
    display.printf("heading  %3d", ballHeading);
    display.setCursor(0, 30);
    display.printf("distance %3d", cam.distance);
    display.setCursor(0, 40);
    display.printf("mag dev  %3d", magDeviation);
  }

  // display the current mode
  display.setCursor(0, 0);
  display.printf("%s", mode);
  if (idle) {
    display.setCursor(SCREEN_WIDTH-6*4, 0); // Size 1: 6x8 pixel area
    display.printf("%s", "IDLE");
  }
  display.display();
}

bool buttonPressed(int button) {
  bool pressed = false;
  unsigned long current = millis();
  // last press at least PRESS_DELAY ms ago
  if (current > button_timer) {
    int state = digitalRead(button);
    if (state == LOW) {
      pressed = true;
      button_timer = current + PRESS_DELAY;
    }
  }
  return pressed;
}

void changeMode(int m = -1) {
  if (m == -1) {
    mode_num += 1;
    if (mode_num == mode_len) {
      mode_num = 0;
    }
  } else {
    mode_num = m;
  }
  mode = Modes[mode_num];
}

bool move_timeout(int speed, int duration) {
  unsigned long current_time = millis();
  // wait for previous command done
  if (current_time > move_timer) {
    // set new timer
    // rump up/down + duration + extra
    move_timer = current_time + speed * 2 + duration + EXTRA_CORR;
    return true;
  }
  return false;
}

void turn2ball(int deviation) {
  const int duration = 8 * abs(deviation);  // ms per 1 degree
  int speed = MIN_SPEED + 1 * abs(deviation);
  if (speed > max_speed) speed = max_speed;

  if (move_timeout(speed, duration)) {
    int Rside = -1;
    int Lside = 1;
    if (deviation < 0) {
      Rside = 1;
      Lside = -1;
    }
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", Rside * speed, Rside * speed, Lside * speed, Lside * speed, duration);
  }
}

void turn2goal() {
  int duration = cam.goalPosition.deviation * 50;
  int speed = 20;
  if (move_timeout(speed, duration)) {
    // cam deviation to speed delta
    int delta = cam.goalPosition.deviation * 2;
    if (cam.goalPosition.dir == 'L') {
      delta = -delta;
    }
    int Rside = speed - delta;
    int Lside = speed + delta;
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", Rside, Rside, Lside, Lside, duration);
  }
}

void tesTmag(int deviation) {
  const int duration = 8 * abs(deviation);
  int speed = MIN_SPEED + 1 * abs(deviation);
  if (speed > max_speed) speed = max_speed;

  if (move_timeout(speed, duration)) {
    int Rside = -1;
    int Lside = 1;
    if (deviation < 0) {
      Rside = 1;
      Lside = -1;
    }
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", Rside * speed, Rside * speed, Lside * speed, Lside * speed, duration);
  }
}

void turn2mag(int deviation) {
  const int duration = abs(deviation) * 8; // ms per 1 degree
  const int speed = MIN_SPEED + ONE_SPEED;
  const int curve = MIN_SPEED;

  if (move_timeout(speed, duration)) {
    int Rside = speed - curve;
    int Lside = speed + curve;
    if (deviation < 0) {
      Rside = speed + curve;
      Lside = speed - curve;
    }
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", Rside, Rside, Lside, Lside, duration);
  }
}

void turnHook (int deviation) {
  const int duration = abs(deviation) * 20;
  const int speed = max_speed * 1.5; // turn vs move
  const int curve = max_speed * 0.15 * (1+abs(deviation)/90);

  //if (move_timeout(speed, duration)) {
    int Rside = speed - curve;
    int Lside = speed + curve;
    if (deviation < 0) {
      Rside = speed + curve;
      Lside = speed - curve;
    }
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", Rside, Rside, Lside, Lside, duration);
  //}
  delay(speed*2 + duration);
  //shoot();
  kick();
}

void move2ball(int distance) {
  // dir_move_all:max:min:direction:duration
  // max, min: 0..255
  int speed = MIN_SPEED * 2 + 0.4 * distance; //0.2
  if (speed > max_speed) speed = max_speed;
  speed = speed * 1.5; // turn vs move speed
  const int direction = 0;
  int duration = 1 * distance;  // ms
  if (duration > 800) { //500
    duration = 800;
  }
  if (move_timeout(speed, duration)) {
    Serial.printf("dir_move_all:%d:%d:%d:%d\n", speed, MIN_SPEED, direction, duration);
  }
}

void shoot(){
  unsigned long current_time = millis();
  // wait at least SHOOT_DELAY ms
  if (current_time > shoot_timer) {
    shoot_timer = current_time + SHOOT_DELAY;
    Serial2.write("shoot\n");
  }
}

void kick() {
  int duration = 0; //50
  int speed = 150;  //255
  int pause = 5000;
  if (move_timeout(speed, duration + pause)) {
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", speed, speed, speed, speed, duration);
  }
  delay(speed*2 + duration); // cmd delay?
  shoot();
}

void getCompass() {

  sBmm150MagData_t magData = bmm150.getGeomagneticData();

  float x = magData.x - mag_x_offset;
  float y = magData.y - mag_y_offset;

  int heading = atan2(y, x) * 180.0 / PI;

  int delta = heading - magHeading;
  if (delta > 180) delta -= 360;
  if (delta < -180) delta += 360;

  // Smooth heading
  heading += delta * 0.1;

  // wrap to 0–360
  if (heading < 0) heading += 360;
  if (heading >= 360) heading -= 360;

  heading = 360 - heading;
  magHeading = heading;
}

void message(char m[]) {
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print(m);
  display.display();
  delay(1000);
}

void autoCalibrate() {

  float x_min = 9999, x_max = -9999;
  float y_min = 9999, y_max = -9999;

  unsigned long startTime = millis();
  message("Rotate robot slowly..");
  int speed = 15;

  if (move_timeout(speed, CALIBRATION_TIME)) {
    Serial.printf("speed4:%d:%d:%d:%d:%d\n", -speed, -speed, speed, speed, CALIBRATION_TIME);
  }

  while (millis() - startTime < CALIBRATION_TIME) {

    sBmm150MagData_t magData = bmm150.getGeomagneticData();

    float x = magData.x;
    float y = magData.y;

    if (x < x_min) x_min = x;
    if (x > x_max) x_max = x;
    if (y < y_min) y_min = y;
    if (y > y_max) y_max = y;

    delay(100);
  }

  mag_x_offset = (x_max + x_min) / 2.0;
  mag_y_offset = (y_max + y_min) / 2.0;
  message("Calibration complete");
}

void saveCalibration() {
  prefs.putFloat("x_off", mag_x_offset);
  prefs.putFloat("y_off", mag_y_offset);
  prefs.putBool("calibrated", true);
  message("Calibration saved");
}

void getLine() {
  line_right = false;
  int val = analogRead(LINE_RIGHT);
  if (val > LINE_THRESHOLD) line_right = true;
  line_left = false;
  val = analogRead(LINE_LEFT);
  if (val > LINE_THRESHOLD) line_left = true;
}

void setup() {

  Wire.begin(SDA_PIN, SCL_PIN);

  #if BOT_ID == 1
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  #else
    display.begin(0x3C);
  #endif

  // Size 1: 6x8 pixel area
  // Size 2: 12x16 pixels
  // display.setTextSize(2);
  display.setRotation(2);
  display.setTextColor(WHITE, BLACK);
  sprintf(msg, "CoreX bot%d", BOT_ID);
  message(msg);

  Serial.begin(9600, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
  Serial.println();
  Serial2.begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);
  Serial2.println();

  pinMode(MUX_S3, OUTPUT);  // set MUX_Sx as output
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S0, OUTPUT);

  pinMode(MODE_BUTTON, INPUT_PULLUP);  // buttons
  pinMode(START_BUTTON, INPUT_PULLUP);

  // zero sensor angle
  angs[0] = 90 + SEGMENT_ANGLE/2;
  // sensors angles
  for (int i = 1; i < 15; i++) {
    angs[i] = angs[i-1] + SEGMENT_ANGLE;
    if (angs[i] >= 360) {
      angs[i] = angs[i] - 360;
    }
  }
  // init cam request
  Serial2.write("get_pos\n");

  // compass
  #if BOT_ID == 1
  #define W2_SDA 13
  #else
  #define W2_SDA 12
  #endif
  #define W2_SCL 14

  Wire1.begin(W2_SDA, W2_SCL);
  if (bmm150.begin() != BMM150_OK) {
    message("BMM150 init failed!");
  }

  bmm150.setOperationMode(BMM150_POWERMODE_NORMAL);
  bmm150.setPresetMode(BMM150_PRESETMODE_HIGHACCURACY);
  bmm150.setMeasurementXYZ(MEASUREMENT_X_ENABLE, MEASUREMENT_Y_ENABLE, MEASUREMENT_Z_DISABLE);

  prefs.begin("compass", false);

  if (prefs.getBool("calibrated", false)) {
    // Load stored offsets
    mag_x_offset = prefs.getFloat("x_off", 0);
    mag_y_offset = prefs.getFloat("y_off", 0);
    message("Calibration loaded");
    sprintf(msg, "x offset: %.2f", mag_x_offset);
    message(msg);
    sprintf(msg, "y offset: %.2f", mag_y_offset);
    message(msg);
  } else {
    message("No calibration found.");
    message("Auto calibration...");
    autoCalibrate();
    saveCalibration();
  }
}

void loop() {

  if (Serial2.available()) {
    getCAM();
    Serial2.write("get_pos\n");
  }

  calcBallHeading();
  ballDeviation = ballHeading;
  if (ballDeviation > 180) ballDeviation -= 360;
  getCompass();
  magDeviation = magChase - magHeading;
  if (magDeviation > 180) magDeviation -= 360;
  if (magDeviation < -180) magDeviation += 360;
  if (line_active) getLine();

  if (buttonPressed(MODE_BUTTON)) {
    if (idle) changeMode();
  }
  if (buttonPressed(START_BUTTON)) {
    if (idle) {
      idle = false;
      changeMode(mode_num); // run selected mode
      if (mode == "CHASE_BALL" || mode == "CHASE_HALF" || mode == "SEARCH_MAG") magChase = magHeading;
      else if (mode == "SPEED_1" || mode == "SPEED_2" || mode == "SPEED_3") {
        int selected_speed = mode_num-(mode_len-4); // 1, 2 or 3
        max_speed = MIN_SPEED + ONE_SPEED * selected_speed;
        sprintf(msg, "Speed %d selected", selected_speed);
        message(msg);
        idle = true;
      } else if (mode == "LINE_ACTIVE") {
        if (line_active) {
          line_active = false;
          line_right = false;
          line_left = false;
          message("Line deactivated");
        } else {
          line_active = true;
          message("Line ***active***");
        }
        idle = true;
      }
    } else {
      idle = true; // off
    }
  }

  refreshDisplay();

  // actions
  if (!idle) {

    int accept_distance = 0.6 * ACPT_DISTANCE;
    if (abs(magDeviation) < 90) accept_distance += ACPT_DISTANCE * (1 - abs(magDeviation)/90);

    // lines
    if (line_right || line_left) {
      int duration = 100;
      if (move_timeout(max_speed, duration)) {
        // move back
        Serial.printf("speed4:%d:%d:%d:%d:%d\n", -max_speed, -max_speed, -max_speed, -max_speed, duration);
      }
    } else {

      if (mode == "BALL_SEARCH") {
        turn2ball(ballDeviation);
      } else if (mode == "DISTANCE_READ") {
        move2ball(cam.distance);
      } else if (mode == "GOAL_SEARCH") {
        turn2goal();
      } else if (mode == "BALL_CHASE") {
        if (abs(ballDeviation) > ACPT_DEVIATION) {
          turn2ball(ballDeviation);
        } else if (cam.distance > ACPT_DISTANCE) {
          move2ball(cam.distance);
        } else {//if (abs(magDeviation) > ACPT_DEVIATION) {
          //turn2mag(magDeviation);
          turnHook(magDeviation);
        /*} else if (cam.goalPosition.deviation > 5) {
          turn2goal();
        } else {
          kick();*/
        }
      } else if (mode == "BALL_HOOK") {
        /*if (abs(ballDeviation) > ACPT_DEVIATION) {
          turn2ball(ballDeviation);
        } else if (cam.distance > ACPT_DISTANCE) {
          move2ball(cam.distance);
        } else {*/
          turnHook(magDeviation);
        //}
      //robot 2
      } else if (mode == "GOAL_KEEPER") {
        if (abs(ballDeviation) > MAX_BALL_DEVIATION) {
          int speed = ballDeviation * 0.05 * max_speed;
          if (speed > max_speed) speed = max_speed;
          if (speed < -max_speed) speed = -max_speed;
          int duration = abs(ballDeviation) * 10;
          if (move_timeout(abs(speed), duration)) {
            int corr = 0.1 * speed;
            if (ballDeviation < 0 ) corr = corr * 0.5; // left compensation
            Serial.printf("speed4:%d:%d:%d:%d:%d\n", -(speed+corr), speed-corr, -(speed-corr), speed+corr, duration);
          }
        }
      } else if (mode == "CALIBRATION") {
        message("Auto calibration...");
        autoCalibrate();
        saveCalibration();
        idle = true;
      } else if (mode == "SEARCH_MAG") {
        if (abs(magDeviation) > ACPT_DEVIATION) {
          tesTmag(magDeviation);
        }
      } else if (mode == "CHASE_HALF") {
        if (abs(ballDeviation) > ACPT_DEVIATION) {
          turn2ball(ballDeviation);
        } else if (cam.distance > ACPT_DISTANCE) {
          move2ball(cam.distance);
        } else {
          changeMode(0); // search mag
        }
      }
    }
  }

  // cycle end
  delay(1);
}