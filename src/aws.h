#ifndef AWSCLIENT_H
#define AWSCLIENT_H

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "uart.h"

// ——— Handlers & setup —————————————————————————————————————————
/**
 * @brief  MQTT message callback
 * @param  topic   topic string
 * @param  payload raw message bytes
 * @param  length  number of bytes in payload
 */
void messageHandler(char* topic, byte* payload, unsigned int length);

/**
 * @brief  Connects to Wi-Fi and then to AWS IoT (MQTT over TLS)
 */
void connectAWS();

#endif 
