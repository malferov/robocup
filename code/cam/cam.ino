#include "esp_camera.h"

typedef struct {
  int r, g, b;
} color_t;

typedef struct {
  char dir;
  int val;
} position_t;

const int width = 96;
const int horizon = 48;
const int threshold = 8;
const color_t no_color = color_t(0,0,0); //black
const position_t zero_position = position_t('L', 0);
const int buttonPin = 13;

color_t g_goal_color = no_color;

void camSetup();

void setup() {
  Serial.begin(9600);
  Serial.println();
  camSetup();
  pinMode(buttonPin, INPUT_PULLUP);
}

color_t get_color(uint8_t * buf, int offset){
  int byte1 = buf[offset];
  int byte2 = buf[offset+1];
  int r = byte1 >> 3;
  int g = ((byte1 & B111) << 3) + (byte2 >> 5);
  int b = byte2 & B11111;
  return color_t(r, g, b);
}

bool is_color(color_t sample, color_t color){
  return (abs(sample.r-color.r) < threshold && abs(sample.g-color.g) < threshold && abs(sample.b-color.b) < threshold);
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
  color_t color = get_color(fb -> buf, center);
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
    color_t color = get_color(fb -> buf, offset + i*2);
    if (left == 0 && is_color(color, goal_color)){
      left = i;
    }
    if (is_color(color, goal_color)){
      right = i;
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
  // debug
  Serial.printf("left %d right %d center %d goal_color %d %d %d ", left, right, center, goal_color.r, goal_color.g, goal_color.b);
  Serial.printf("position %c%d\n", position.dir, position.val);
  return position;
}

bool buttonPressed(){
  // check if pressed for at least 100 ms
  bool pressed = true;
  for (int i=0; i<10; i++) {
    int state = digitalRead(buttonPin);
    if (state == HIGH){
      pressed = false;
    }
    delay(10);
  }
  return pressed;
}

void loop() {
  if (Serial.available()) {
    String received = Serial.readStringUntil('\n');
    received.trim();
    if (received == "get_pos") {
      position_t position = get_goal_position(g_goal_color);
      Serial.printf("%c:%d\n", position.dir, position.val);
    }
  }
  if (buttonPressed()) {
    g_goal_color = capture_color();
    //debug
    //Serial.printf("button pressed; goal color %d %d %d\n", g_goal_color.r, g_goal_color.g, g_goal_color.b);
  }
  //debug
  //position_t position = get_goal_position(g_goal_color);
  //Serial.printf("%c:%d\n", position.dir, position.val);
  //delay(200);
}
