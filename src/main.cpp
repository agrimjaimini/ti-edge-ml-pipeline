#include <Arduino.h> 
#include "aws.h"
void setup()
{
  Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1,D0,D1); // matches 8 bit no parity
   connectAWS(); 
}
 
void loop()
{ 
  publishMessage();
  client.loop();
  delay(2500);
}