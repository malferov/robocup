#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 21
#define SCL_PIN 22

#define SERIAL_RX 3
#define SERIAL_TX 1
#define SERIAL2_RX 16
#define SERIAL2_TX 17

#define MUX_SIG 15
#define MUX_S3 2
#define MUX_S2 4
#define MUX_S1 5
#define MUX_S0 18

#define ANALOG_MAX 4095
#define SEGMENT_ANGLE 24

#define MODE_BUTTON 19
#define START_BUTTON 23
#define PRESS_DELAY 500  // ms

#define BALL_CLOSE 100 // mm

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
CAMT cam = CAMT(zeroPosition, -1);
int ballHeading[2] = {0, 0};   // average & last
int angs[15];         // sensors angles

String Modes[] = {"IDLE","BALL_SEARCH","DISTANCE_READ","GOAL_SEARCH","BALL_CHASE","IR_READ"};

int mode_num = 0;
String mode = Modes[0];
unsigned long timer = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  // Size 1: 6x8 pixel area
  // Size 2: 12x16 pixels
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 20);  // Move text to new origin
  display.print("CoreX bot1");
  display.display();
  delay(1000);

  Serial.begin(9600, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
  Serial2.begin(9600, SERIAL_8N1, SERIAL2_RX, SERIAL2_TX);

  pinMode(MUX_S3, OUTPUT);  // set MUX_Sx as output
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S0, OUTPUT);

  pinMode(MODE_BUTTON, INPUT_PULLUP);  // buttons
  pinMode(START_BUTTON, INPUT_PULLUP);
  timer = millis();

  // zero sensor angle
  angs[0] = 90 + SEGMENT_ANGLE/2;
  // add 180 correction
  angs[0] = angs[0] + 180;
  // sensors angles
  for (int i = 1; i < 15; i++) {
    angs[i] = angs[i-1] + SEGMENT_ANGLE;
    if (angs[i] >= 360) {
      angs[i] = angs[i] - 360;
    }
  }
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

  int angle = ballHeading[1];
  int irs[15];
  int sum = 0;
  int cnt = 0;
  // get current angle
  for (int i = 0; i < 15; i++) {
    irs[i] = getIR(i);
    if (irs[i] == 1) {
      cnt++;
      sum = sum + angs[i];
    }
  }
  if (cnt > 0) {
    angle = sum / cnt;
  }
  // calculate average
  ballHeading[0] = (angle + ballHeading[1]) / 2;
  // remember last
  ballHeading[1] = angle;
}

CAMT getCAM() {
  String command = Serial2.readStringUntil('\n');
  if (command[0] == 'L' || command[0] == 'R') {
    char dir = getValue(command,':',0)[0];
    int deviation = getValue(command,':',1).toInt();
    int left = getValue(command,':',2).toInt();
    int right = getValue(command,':',3).toInt();
    int center = getValue(command,':',4).toInt();
    int distance = getValue(command,':',5).toInt();
    return CAMT(goalPositionT(dir, deviation, left, right, center), distance);
  } else {
    return CAMT(zeroPosition, -1);
  }
}

void refreshDisplay() {
  display.clearDisplay(); //clear display
  // display the data corresponding to the mode
  if (mode == "BALL_SEARCH") {
    // ball heading
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print(ballHeading[0]);
  } else if (mode == "DISTANCE_READ") {
    // distance
    display.setCursor(SCREEN_WIDTH - 12*3, SCREEN_HEIGHT - 16); // size 2: 12x16
    display.printf("%03d", cam.distance);
  } else if (mode == "GOAL_SEARCH") {
    // camera
    int difference = abs(cam.goalPosition.left - cam.goalPosition.right);
    display.drawRect((SCREEN_WIDTH - difference) / 2, (SCREEN_HEIGHT - 30) / 2, difference, 30, SSD1306_WHITE);
    display.fillRect(cam.goalPosition.center, (SCREEN_HEIGHT - 50) / 2, 10, 50, SSD1306_WHITE);
  } else if (mode == "BALL_CHASE") {

  } else if (mode == "IR_READ") {
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print(ballHeading[0]);
  }
  // display the current mode
  display.setCursor(0, 20);
  display.setTextSize(1);
  display.print(mode);
  display.display();
}

bool buttonPressed(int button) {
  bool pressed = false;
  unsigned long current = millis();
  // last press at least PRESS_DELAY ms ago
  if (current > timer + PRESS_DELAY) {
    int state = digitalRead(button);
    // if pressed
    if (state == LOW) {
      pressed = true;
      timer = current;
    }
  }
  return pressed;
}

void changeMode() {
  mode = Modes[mode_num + 1];
  mode_num += 1;
}

void turnAll(char dir, int speed) {
  if (speed > 255) {
    speed = 255;
  }
  const int duration = 3; // ms
  if (dir == 'L') {
    Serial.printf("turn_all:%d:0:1:%d\n", speed, duration);
  } else if (dir == 'R') {
    Serial.printf("turn_all:%d:0:0:%d\n", speed, duration);
  }
}

void turn2ball() {
  char direction = 'L';
  if (ballHeading[0] > 180) {
    direction = 'R';
  }
  int deviation = abs(180 - ballHeading[0]);  // degrees
  turnAll(direction,30);
}

void turn2goal() {
  if (cam.goalPosition.deviation == 0) {
    return;
  }
  turnAll(cam.goalPosition.dir, cam.goalPosition.deviation * 10 ); // cam deviation to degrees
}

void move2ball(int distance) {
  // dir_move_all:max:min:direction:duration
  // max, min: 0..255
  int speed = 0;
  if (distance <= 100) {
    speed = 20;
  } else {
    speed = 40;
  }
  const int direction = 0; // [0, 90, 180, 270] degrees
  const int duration = 3;  // ms
  Serial.printf("dir_move_all:%d:0:%d:%d\n", speed, direction, duration);
}

void shoot(){
  display.setCursor(0, 20);
  display.print("shoot");
  display.display();
  Serial2.write("shoot\n");
  delay(200);
}

void loop() {

  if (Serial2.available()) {
    cam = getCAM();
  }
  Serial2.write("get_pos\n");

  calcBallHeading();

  if (buttonPressed(MODE_BUTTON)) {
    changeMode();
  }
  if (buttonPressed(START_BUTTON)) {
    shoot();
  }

  refreshDisplay();

  // actions
  if (mode == "BALL_SEARCH") {
    if (abs(ballHeading[0] - 180) > 5) {
      turn2ball();
    }
  } else if (mode == "DISTANCE_READ") {
    if (cam.distance > 35) {
      move2ball(cam.distance);
    } else {
      shoot();
    }
  } else if (mode == "GOAL_SEARCH") {
    // //turn2goal();
    // shoot();
    // mode = "IDLE";
  }
  delay(50);
}
