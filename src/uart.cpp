// Prathik Narsetty
/*
This file is to let the user dictate the type of data they are trying to push to the cloud via our pipeline
*/

#include "uart.h"
#include "JSONFORMAT.h"
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
 
using namespace std;

void publishMessage()
{
  JsonDocument doc;

  //// THIS SECTION SHOULD BE EDITED TO MATCH INCOMING PARAMS /////////////////////
  int z = 0;
  int ti = millis();
    while (Serial2.available()< NUMPARAM) { // waits for data for all elemtns to be here
           
    if (millis()-ti > TIMEOUT_MS) { /// clears buffer if it was a timeout
      while (Serial2.available()) {
    Serial2.read();
  }
  return; //   
    }

    }


    /// HERE!!!!
    FIELD_LIST;
    

while (Serial2.available()) {
    Serial2.read();
  }
  char jsonBuffer[1024]; 
  /// 
serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer); // only publishes the amount of the buffer which is used 
}