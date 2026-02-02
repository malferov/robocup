#include "esp_camera.h"
#define BUTTON_PIN 47
#define PRESS_DELAY 500 // ms
#define TRIG_PIN 19
#define ECHO_PIN 20
#define ECHO_TIMEOUT 50000 // us
#define STATUS_LED 2

typedef struct {
  int r, g, b;
} color_t;

typedef struct {
  char dir;
  int val;
  int left;
  int right;
  int center;
} position_t;

const int width = 96;
const int horizon = 48;
const int threshold_color = 5;
const int threshold_width = 10;
const color_t no_color = color_t(0,0,0); //black
const position_t zero_position = position_t('L',0,0,0,0);
unsigned long timer = 0;

color_t g_goal_color = no_color;

void camSetup();

void setup() {
  // button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  timer = millis();
  // distance sensor
  pinMode(ECHO_PIN, INPUT_PULLDOWN);
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(TRIG_PIN, OUTPUT);
  // led
  pinMode(STATUS_LED, OUTPUT);
  // serial
  Serial.begin(9600);
  Serial.println();
  // camera
  camSetup();

  // debug info
  /*
  if (psramFound()) {
    Serial.println("PSRAM FOUND");
    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
  } else {
    Serial.println("PSRAM NOT FOUND");
  }
  sensor_t *s = esp_camera_sensor_get();
  Serial.printf("Camera PID: %X\n", s->id.PID);
  */
}

color_t get_color(uint8_t * buf, int offset){
  int byte1 = buf[offset];
  int byte2 = buf[offset+1];
  int r = byte1 >> 3;
  int g = ((byte1 & B111) << 3) + (byte2 >> 5);
  int b = byte2 & B11111;
  return color_t(r, g, b);
}

color_t get_avg_color(uint8_t * buf, int offset){
  // avg of 5 lines above and 5 lines below
  int five_lines_offset = width * 2 * 5;
  color_t color_above = get_color(buf, offset - five_lines_offset);
  color_t color_below = get_color(buf, offset + five_lines_offset);
  color_t avg_color = color_t((color_above.r + color_below.r)/2, (color_above.g + color_below.g)/2, (color_above.b + color_below.b)/2);
  return avg_color;
}

bool is_color(color_t sample, color_t color){
  int diff_r = abs(sample.r-color.r);
  int diff_g = abs(sample.g-color.g);
  int diff_b = abs(sample.b-color.b);
  //debug
  //Serial.printf("%d %d %d\n", diff_r, diff_g, diff_b);
  bool match = (diff_r < threshold_color && diff_g < threshold_color && diff_b < threshold_color);
  return match;
}

color_t capture_color(){
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Frame buffer could not be acquired");
    return no_color; //black
  }
  //color_t yellow = color_t(31, 63, 28);
  int center = width * 2*48 + 2*48; // center pixel (9312)
  color_t color = get_avg_color(fb -> buf, center);
  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);

  // debug
  //int bytes = (fb->buf[9312]<<8) + fb->buf[9312+1];
  //Serial.printf("rgb %d %d %d color %s bytes %04X\n", color.r, color.g, color.b, color_s, bytes);
  return color;
}

position_t get_goal_position(color_t goal_color){
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Frame buffer could not be acquired");
    return zero_position;
  }
  int offset = horizon * width * 2;
  int left=0;
  int right=0;
  for (int i=0; i<width; i++) {
    color_t color = get_avg_color(fb -> buf, offset + i*2);
    if (left == 0 && is_color(color, goal_color)){
      left = i;
    }
    if (is_color(color, goal_color)){
      right = i;
    } else if (right - left < threshold_width) {
      left = 0;
      right = 0;
    }
  }
  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);

  int center = (right + left)/2;
  position_t position;
  if (center > width/2) {
    position.dir = 'R';
  } else {
    position.dir = 'L';
  }
  position.val = 8 * abs(center - width/2) / (width/2);
  if (left == 0 or right == 0){
    position = zero_position;
  }
  position.left = left;
  position.right = right;
  position.center = center;
  // debug
  //Serial.printf("left %d right %d center %d goal_color %d %d %d ", left, right, center, goal_color.r, goal_color.g, goal_color.b);
  //Serial.printf("position %c%d\n", position.dir, position.val);

  return position;
}

bool buttonPressed() {
  bool pressed = false;
  unsigned long current = millis();
  if (current > timer + PRESS_DELAY) {
    int state = digitalRead(BUTTON_PIN);
    if (state == LOW) {
      pressed = true;
      timer = current;
    }
  }
  return pressed;
}

int getDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int distance = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT) * 0.1715; //    0.343/2
  return distance;
}

void confirm() {
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
}

void loop() {
  if (Serial.available()) {
    String received = Serial.readStringUntil('\n');
    received.trim();
    if (received == "get_pos") {
      position_t position = get_goal_position(g_goal_color);
      int distance = getDistance(); // mm
      Serial.printf("%c:%d:%d:%d:%d:%d\n", position.dir, position.val, position.left, position.right, position.center, distance);
    } else if (received == "kick") {
      digitalWrite(STATUS_LED, HIGH);
      delay(500);
      digitalWrite(STATUS_LED, LOW);
    }
  }
  if (buttonPressed()) {
    g_goal_color = capture_color();
    confirm();
    //debug
    //Serial.printf("button pressed; goal color %d %d %d\n", g_goal_color.r, g_goal_color.g, g_goal_color.b);
  }
  delay(1);

  //debug
  //position_t position = get_goal_position(g_goal_color);
  //Serial.printf("%c:%d:%d:%d:%d\n", position.dir, position.val, position.left , position.right, position.center);
}
