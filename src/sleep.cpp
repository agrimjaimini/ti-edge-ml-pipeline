#include <Arduino.h>
// code for sleep 


void Sleepsetup() {
pinMode(LED_BUILTIN, OUTPUT);
digitalWrite(LED_BUILTIN, HIGH);            // Turn on LED to indicate boot
delay(2000);                                // Wait 2 seconds before sleeping
digitalWrite(LED_BUILTIN, LOW);
    //  esp_sleep_enable_timer_wakeup(10000000);
  // /  / prepare for sleep
esp_sleep_enable_ext1_wakeup(0x04, ESP_EXT1_WAKEUP_ANY_HIGH);
  // // Enable sleep
   esp_deep_sleep_start();
}

void SLeeploop() {
    // nothing ever runs here
}