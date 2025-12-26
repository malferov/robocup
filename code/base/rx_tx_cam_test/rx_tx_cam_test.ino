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
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.begin(9600, SERIAL_8N1, 3, 1);
}

void loop() {
  Serial2.write("get_pos\n");
  if (Serial2.available()) {
    String command = Serial2.readStringUntil('\n');
    if (getValue(command,':',0) == "L") {
      Serial.printf("turn_all:%d:0:1:100\n",getValue(command,':',1).toInt()*10);
    } else if (getValue(command,':',0) == "R") {
      Serial.printf("turn_all:%d:0:0:100\n",getValue(command,':',1).toInt()*10);
    }
  }
  
  delay(100);
}
