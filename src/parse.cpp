// #include <Arduino.h>
//     uint16_t data = 0x0000;
// void setup () {

//   Serial.begin(115200);
//     Serial2.begin(115200, SERIAL_8N1,D0,D1); // matches 8 bit no parity
// }
// void loop () {
//     if (!Serial2.available()) {
//         return;
//     }
//     data |= Serial2.read();
//     data = data<<8;
//     Serial.printf("0x%04X\n", data);
// }