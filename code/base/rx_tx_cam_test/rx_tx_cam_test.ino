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

typedef struct {
  char dir;
  int deviation;
  int left;
  int right;
  int center;
} goalPositionT;

goalPositionT zeroPosition = goalPositionT('L',0,0,0,0);
goalPositionT goalPosition = zeroPosition;
int ballHeading = 0;

#define MODE_GOAL 0
#define MODE_BALL 1
int mode = MODE_GOAL;
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
  display.setTextSize(2);     // Larger text
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
}

int getIR(int channel) {
  digitalWrite(MUX_S3, (channel >> 3) & 0x1);   // select channel
  digitalWrite(MUX_S2, (channel >> 2) & 0x1);
  digitalWrite(MUX_S1, (channel >> 1) & 0x1);
  digitalWrite(MUX_S0, channel & 0x1);
  int val = analogRead(MUX_SIG);
  //float valf = (ANALOG_MAX - val) / ANALOG_MAX * 100;   // normalization
  return val;
}

int getBallHeading() {
  int channel = -1;   // unknown position
  int min = ANALOG_MAX;
  for (int i = 0; i < 15; i++) {
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

  int angle = 8 * SEGMENT_ANGLE - 90;           // initial offset
  angle = angle + SEGMENT_ANGLE * channel;  // add channel position
  if (angle > 360) {
    angle = angle - 360;            // whole round offset
  } 
  return angle;
}

goalPositionT getGoalPosition() {
  String command = Serial2.readStringUntil('\n');
  if (command[0] == 'L' || command[0] == 'R') {
    char dir = getValue(command,':',0)[0];
    int deviation = getValue(command,':',1).toInt();
    int left = getValue(command,':',2).toInt();
    int right = getValue(command,':',3).toInt();
    int center = getValue(command,':',4).toInt();
    return goalPositionT(dir, deviation, left, right, center);
  } else {
    return zeroPosition;
  }
}

void turnAll(char dir, int deviation) {
  int speed = 20;
  if (dir == 'L') {
    Serial.printf("turn_all:%d:0:1:100\n", deviation * speed);
  } else if (dir == 'R') {
    Serial.printf("turn_all:%d:0:0:100\n", deviation * speed);
  }
}

void refreshDisplay() {
  int difference = abs(goalPosition.left - goalPosition.right);
  int x = (SCREEN_WIDTH - difference) / 2;
  int y = (SCREEN_HEIGHT - 30) / 2;

  display.clearDisplay();
  display.drawRect(x, y, difference, 30, SSD1306_WHITE);
  y = (SCREEN_HEIGHT - 50) / 2;
  display.fillRect(goalPosition.center, y, 10, 50, SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(ballHeading);
  display.setCursor(SCREEN_WIDTH - 12, 0); // 12 width of one char
  String strmod = "G";
  if (mode == MODE_BALL) {
    strmod = "B";
  }
  display.print(strmod);
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
  if (mode == MODE_GOAL) {
    mode = MODE_BALL;
  } else {
    mode = MODE_GOAL;
  }
}

void turn2ball() {
  if (ballHeading == -1) {
    return;
  }
  char direction = 'L';
  if (ballHeading < 180) {
    direction = 'R';
  }
  int heading = ballHeading;
  if (heading > 180) {
    heading = 360 - heading;
  } 
  turnAll(direction, heading / SEGMENT_ANGLE);
}

void loop() {
  if (Serial2.available()) {
    goalPosition = getGoalPosition();
  }
  Serial2.write("get_pos\n");
  ballHeading = getBallHeading();
  if (buttonPressed(MODE_BUTTON)) {
    changeMode();
  }
  refreshDisplay();
  if (mode == MODE_BALL) {
    turn2ball();
  }
  delay(100);
}
