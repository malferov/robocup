#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


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

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.begin(9600, SERIAL_8N1, 3, 1);
}

void loop() {
  Serial2.write("get_pos\n");
  if (Serial2.available()) {
    String command = Serial2.readStringUntil('\n');
    if (getValue(command,':',0) == "L") {
      Serial.printf("turn_all:%d:0:0:100\n",getValue(command,':',1).toInt()*20);
    } else if (getValue(command,':',0) == "R") {
      Serial.printf("turn_all:%d:0:1:100\n",getValue(command,':',1).toInt()*20);
    }
    int left = getValue(command,':',2).toInt();
    int right = getValue(command,':',3).toInt();
    int center = getValue(command,':',4).toInt();
    int difference = abs(left-right);
    display.clearDisplay();

    int x = (SCREEN_WIDTH - difference) / 2;
    int y = (SCREEN_HEIGHT - 30) / 2;
    display.drawRect(x, y, difference, 30, SSD1306_WHITE);


    y = (SCREEN_HEIGHT - 50) / 2;
    display.fillRect(center, y, 10, 50, SSD1306_WHITE);

    display.display();
  }
  
  delay(100);
}
