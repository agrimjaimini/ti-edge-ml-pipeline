#include <Arduino.h> 
#include "aws.h"
void setup()
{
  Serial.begin(115200);
    Serial2.begin(1250000, SERIAL_8N1,D0,D1); // matches 8 bit no parity
  //  connectAWS(); 
}

uint32_t window = 0;
void loop() {
  if (Serial2.available() == 0) return;

  int b = Serial2.read();
  if (b < 0) return;
  window = (window << 8) | uint8_t(b);

  // raw on-the-wire compare (0x02 then 0x01)


  // or, if you really want to treat that as LE 0x0102:
  // for 16 bits
  // uint16_t le = (window >> 8) | (window << 8);
  // if (le == 0x0102) {
  //   Serial.println("Magic LE 0x0102 detected!");
  // }
  // if (le == 0x0304) {
  //     Serial.println("Magic LE 0x0203 detected!");  
  // }
  // for 32 bits
  uint32_t rotatedWindow = (((window&0xFF000000)>>24) | ((window&0x00FF0000)>>8) | ((window&0x0000FF00) <<8) | ((window&0x000000FF)<< 24)); 
  // if (rotatedWindow == 0x0000012D) {
  //   Serial.print("301");
  // }
  // if (rotatedWindow == 0x0000012E) {
  //   Serial.print("302");
  // }
  // if (rotatedWindow == 0x0000012F) {
  //   Serial.print("303");
  // }
  //   if (rotatedWindow == 0x00000130) {
  //   Serial.print("304");
  // }
  // if (rotatedWindow == 0x00000131) {
  //   Serial.print("305");
  // }
  // if (rotatedWindow == 0x00000132) {
  //   Serial.print("306");
  // }
  //   if (rotatedWindow == 0x00000133) {
  //   Serial.print("307");
  // }
  // if (rotatedWindow == 0x00000134) {
  //   Serial.print("308");
  // }
  // if (rotatedWindow == 0x00000135) {
  //   Serial.print("309");
  // }
  //      Serial.printf("0x%04X\n", rotatedWindow);
}

