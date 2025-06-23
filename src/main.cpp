#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#include "uart.h"
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
 
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
    
  JsonDocument doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
// void publishMessage()
// {
//   JsonDocument doc;
//   doc["data"] = buttonState;
//  // 1257815, 5, 1, 3, 1, 2, 2, 1, 1, 6, 4, 3, 2, 5, 5, 6, 5, 4, 4, 23, 6, 3, 2, 7, 8, 8, 3, 2, 1, 6;
//   char jsonBuffer[512];
// serializeJson(doc, jsonBuffer); // print to client
 
//   client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer); // only publishes the amount of the buffer which is used 
// }

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






// ///////////////

// // code for sleep 


// void setup() {
// pinMode(LED_BUILTIN, OUTPUT);
// digitalWrite(LED_BUILTIN, HIGH);            // Turn on LED to indicate boot
// delay(2000);                                // Wait 2 seconds before sleeping
// digitalWrite(LED_BUILTIN, LOW);
//     //  esp_sleep_enable_timer_wakeup(10000000);
//   // /  / prepare for sleep
// esp_sleep_enable_ext1_wakeup(0x04, ESP_EXT1_WAKEUP_ANY_HIGH);
//   // // Enable sleep
//    esp_deep_sleep_start();
// }

// void loop() {
//     Serial.print("hh");
//     delay(500);
// }