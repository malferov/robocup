#include "esp_camera.h"
// debug
//#include "mbedtls/base64.h"

#define BUTTON_PIN 47
#define PRESS_DELAY 500 // ms
#define TRIG_PIN 19
#define ECHO_PIN 20
#define ECHO_TIMEOUT 50000 // us
#define STATUS_LED 2
#define MOSFET_PIN 38

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

//const int width = 96;
//const int horizon = 96/2;
const int width = 160;
const int horizon = 120 * 0.52;
// 0.51 0.75 of the field
// 0.52 0.50 of the field
// 0.53 0.25 of the field
const int threshold_color = 5;
const int threshold_width = 7;
const int threshold_border = 3;
const color_t no_color = color_t(0,0,0); //black
const position_t zero_position = position_t('L',0,0,0,0);
position_t last_known_position = zero_position;
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
  // MOSFET
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);
  // serial
  Serial.begin(9600);
  //Serial.begin(115200);
  Serial.println();
  // camera
  camSetup();

  // ESP32S3 Dev Module
  // Tools - PSRAM - OPI PSRAM 
  // debug info
  /*if (psramFound()) {
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

color_t get_color(uint8_t * buf, int offset) {
  int byte1 = buf[offset];
  int byte2 = buf[offset+1];
  int r = byte1 >> 3;
  int g = ((byte1 & B111) << 3) + (byte2 >> 5);
  int b = byte2 & B11111;
  return color_t(r, g, b);
}

color_t get_avg_color(uint8_t * buf, int offset) {
  // average of pixels before and after the point
  int pixels_offset = 2;
  color_t color_point = get_color(buf, offset);
  color_t color_before = get_color(buf, offset - pixels_offset);
  color_t color_after = get_color(buf, offset + pixels_offset);
  color_t avg_color = color_t((color_before.r + color_point.r + color_after.r)/3, (color_before.g + color_point.g + color_after.g)/3, (color_before.b + color_point.b + color_after.b)/3);
  return avg_color;
}

bool is_color(color_t sample, color_t color) {
  int diff_r = abs(sample.r-color.r);
  int diff_g = abs(sample.g-color.g);
  int diff_b = abs(sample.b-color.b);
  //debug
  //Serial.printf("%d %d %d\n", diff_r, diff_g, diff_b);
  bool match = (diff_r < threshold_color && diff_g < threshold_color && diff_b < threshold_color);
  return match;
}

color_t capture_color() {
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Frame buffer could not be acquired");
    return no_color; //black
  }
  //color_t yellow = color_t(31, 63, 28);
  int center = width * horizon * 2 + width; // center point
  color_t color = get_avg_color(fb -> buf, center);
  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);

  // debug
  //int bytes = (fb->buf[9312]<<8) + fb->buf[9312+1];
  //Serial.printf("rgb %d %d %d color %s bytes %04X\n", color.r, color.g, color.b, color_s, bytes);
  return color;
}

position_t get_goal_position(color_t goal_color) {
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Frame buffer could not be acquired");
    return last_known_position;
  }
  int offset = horizon * width * 2;
  int left=0;
  int left_cnt=0;
  int right=0;
  int right_cnt=0;
  for (int i=0; i<width; i++) {
    color_t color = get_avg_color(fb -> buf, offset + i*2);
    if (left == 0) {
      if (is_color(color, goal_color)) {
        if (left_cnt < threshold_border) {
          left_cnt++;
        } else {
          left = i - threshold_border;
          right = left;
        }
      } else {
        left_cnt = 0;
      }
    }
    if (left > 0) {
      if (is_color(color, goal_color)) {
        if (right_cnt < threshold_border) {
          right_cnt++;
        } else {
          right = i;
        }
      } else {
        right_cnt = 0;
      }
    }
  }

  // debug
  /*
  #define B64_CHUNK_RAW 300
  static uint8_t out[4 * ((B64_CHUNK_RAW + 2) / 3) + 4];
  Serial.println("===IMAGE START===");
  Serial.printf("SIZE:%u\n", fb->len);
  Serial.printf("WIDTH:%u\n", fb->width);
  Serial.printf("HEIGHT:%u\n", fb->height);
  // Stream Base64 encoding in chunks
  size_t i = 0;
  while (i < fb->len) {
    size_t chunk = B64_CHUNK_RAW;
    if (i + chunk > fb->len)
      chunk = fb->len - i;
    size_t actual_out = 0;
    int ret = mbedtls_base64_encode(
      out,
      sizeof(out),
      &actual_out,
      fb->buf + i,
      chunk
    );
    if (ret != 0) {
      Serial.printf("Base64 encode error: %d\n", ret);
      break;
    }
    Serial.write(out, actual_out);
    i += chunk;
  }
  Serial.println("\n===IMAGE END===");
  */
  // debug end

  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);

  if (right - left < threshold_width) {
    left = 0;
    right = 0;
  } 

  position_t position;
  if (left == 0 && right == 0){
    position = last_known_position;
  } else {
    int center = (right + left)/2;
    if (center > width/2) {
      position.dir = 'R';
    } else {
      position.dir = 'L';
    }
    position.val = 8 * abs(center - width/2) / (width/2);
    position.left = left;
    position.right = right;
    position.center = center;
    last_known_position = position;
  }
  // debug
  //Serial.printf("left %d right %d center %d goal_color %d %d %d ", left, right, center, goal_color.r, goal_color.g, goal_color.b);
  //Serial.printf("position %c%d\n", position.dir, position.val);
  // debug
  /*
  Serial.println("===METRICS START===");
  Serial.printf("GoalColor %d %d %d\n", goal_color.r, goal_color.g, goal_color.b);
  Serial.printf("PositionDir %c\n", position.dir);
  Serial.printf("PositionVal %d\n", position.val);
  Serial.printf("PositionLeft %d\n", position.left);
  Serial.printf("PositionRight %d\n", position.right);
  Serial.printf("PositionCenter %d\n", position.center);
  Serial.println("===METRICS END===");
  */
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

void shoot() {
  digitalWrite(MOSFET_PIN, LOW);
  delay(200);
  digitalWrite(MOSFET_PIN, HIGH);
}

void loop() {
  if (Serial.available()) {
    String received = Serial.readStringUntil('\n');
    received.trim();
    if (received == "get_pos") {
      position_t position = get_goal_position(g_goal_color);
      int distance = getDistance(); // mm
      Serial.printf("%c:%d:%d:%d:%d:%d\n", position.dir, position.val, position.left, position.right, position.center, distance);
    } else if (received == "shoot") {
      shoot();
      confirm();
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
