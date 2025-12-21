#include "esp_camera.h"
#include <WiFi.h>

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

// ===========================
// Enter your WiFi credentials
// ===========================
/*
const char *ssid = "WLNet";
const char *password = "29735945";
void startCameraServer();
*/
typedef struct {
  int r, g, b;
} color_t;

typedef struct {
  char dir;
  int val;
} position_t;

void setupLedFlash();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_96X96;
  config.pixel_format = PIXFORMAT_RGB565;
  //config.pixel_format = PIXFORMAT_RGB888;
  //config.pixel_format = PIXFORMAT_YUV422;
  //config.pixel_format = PIXFORMAT_GRAYSCALE;
  //config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.jpeg_quality = 1;
  //config.pixel_format = PIXFORMAT_RAW;
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  //config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;
/*
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }
*/
#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif
/*
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();
  Serial.println("Camera Ready");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
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

int width = 96;
int horizon = 48;

position_t get_goal_position(uint8_t * buf, color_t goal_color){
  int offset = horizon * width * 2;
  int left=0;
  int right=0;
  for (int i=0; i<width; i++) {
    color_t color = get_color(buf, offset + i*2);
    if (left == 0 && is_color(color, goal_color)){
      left = i;
    }
    if (left > 0 && right == 0 && !is_color(color, goal_color)){
      right = i;
    }
    if (right > 0 && is_color(color, goal_color)){
      right = i;
    }
  }
  int center = (right + left)/2;
  position_t position;
  if (center > width/2) {
    position.dir = 'R';
  } else {
    position.dir = 'L';
  }
  position.val = 8 * abs(center - width/2) / (width/2);
  // debug
  //Serial.printf("left %d right %d center %d position %c%d\n", left, right, center, position.dir, position.val);
  //Serial.printf("center %d position %c%d\n", center, position.dir, position.val);
  return position;
}

int threshold = 8;

bool is_color(color_t sample, color_t color){
  return (abs(sample.r-color.r) < threshold && abs(sample.g-color.g) < threshold && abs(sample.b-color.b) < threshold);
}
esp_err_t camera_capture(){
        //capture a frame
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Frame buffer could not be acquired");
            return ESP_FAIL;
        }

        String color_s;
        color_t black = color_t(5, 11, 6);
        color_t yellow = color_t(31, 63, 28);
        color_t blue = color_t(8, 28, 21);
        int center = width * 2*48 + 2*48; // center pixel (9312)
        color_t color = get_color(fb -> buf, center);

        if (is_color(color, black)) {
          color_s = "black";
        } else if (is_color(color, yellow)) {
          color_s = "yellow";
        } else if (is_color(color, blue)) {
          color_s = "blue";
        }
        // debug
        //int bytes = (fb->buf[9312]<<8) + fb->buf[9312+1];
        //Serial.printf("rgb %d %d %d color %s bytes %04X\n", color.r, color.g, color.b, color_s, bytes);
        position_t position = get_goal_position(fb -> buf, yellow);
        Serial.printf("position %c%d\n", position.dir, position.val);
        
        //return the frame buffer back to be reused
        esp_camera_fb_return(fb);

        return ESP_OK;
}

void loop() {
  esp_err_t err = camera_capture();
  if (err != ESP_OK) {
    Serial.printf("Camera capture failed with error 0x%x", err);
  }
  //Serial.printf("touch %d %d %d\n", touchRead(4), touchRead(2), touchRead(14));
  delay(100);
}
