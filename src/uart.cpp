#include "uart.h"

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

  #define NUMPARAM 6
  #define TIMEOUT_MS 1000

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
 
using namespace std;

void publishMessage()
{
  JsonDocument doc;

  //// THIS SECTION SHOULD BE EDITED TO MATCH INCOMING PARAMS
  int z = 0;
  int ti = millis();
    while (Serial2.available()< NUMPARAM) { 
           
    if (millis()-ti > TIMEOUT_MS) { /// clears buffer if it was a timeout
      while (Serial2.available()) {
    Serial2.read();
  }
  return; //   
    }
    }
        uint8_t c = Serial2.read();
        doc["c"] = c;
    
        uint8_t d = Serial2.read();
        doc["d"] = d;
    
        uint8_t e = Serial2.read();
        doc["e"] = e;
    
        uint8_t f = Serial2.read();
        doc["f"] = f;
    
        uint8_t g = Serial2.read();
        doc["g"] = g;
    
        uint8_t h = Serial2.read();
        doc["h"] = h;
    

while (Serial2.available()) {
    Serial2.read();
  }
  char jsonBuffer[512];


  /// 
serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer); // only publishes the amount of the buffer which is used 
}