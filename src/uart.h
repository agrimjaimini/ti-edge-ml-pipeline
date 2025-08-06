#ifndef UART_H
#define UART_H

#include "secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// MQTT network client and PubSub client
extern WiFiClientSecure net;
extern PubSubClient client;

// Reads up to six bytes from Serial2 and publishes them as JSON
void publishMessage();

#endif 