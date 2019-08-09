#include <SoftwareSerial.h>

int incomingByte = 0; // for incoming serial data

SoftwareSerial HoverBoard (8, 9); // RX, TX

void setup() {
  Serial.begin(19200);
  HoverBoard.begin(115200);
//  HoverBoard.begin(19200);

  Serial.println("started");
}

/**/
void loop() {
  String storedData = "";
  while(HoverBoard.available()) {
    delay(2);
    char inChar = HoverBoard.read();
    storedData += inChar;
  }
  if (storedData.length() > 0) {
    Serial.println(storedData);
    storedData = "";
  } else {
    Serial.println("no data");
  }
  delay(100);
}
/**/

/*
uint8_t speed = 1;
uint8_t steer = 0;

void loop(void){
  if (speed = 1) {
    speed = 0;
  } else {
    speed = 1;
  }
  
  Serial.write((uint8_t *) &steer, sizeof(steer));
  Serial.write((uint8_t *) &speed, sizeof(speed));
  delay(2000);
}
*/
