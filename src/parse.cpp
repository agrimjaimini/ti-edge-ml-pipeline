
//   #include <Arduino.h> 
//   #include "aws.h"
//   void setup()
//   {
//     Serial.begin(115200);
//       Serial2.begin(1250000, SERIAL_8N1,D0,D1); // matches 8 bit no parity
//     //  connectAWS(); 
//   }

//   uint32_t window = 0;
//   void loop() {
//     if (Serial2.available() == 0) return;

//     int b = Serial2.read();
//     if (b < 0) return;
//     window = (window << 8) | uint8_t(b);


//     // for 32 bits
//     uint32_t rotatedWindow = (((window&0xFF000000)>>24) | ((window&0x00FF0000)>>8) | ((window&0x0000FF00) <<8) | ((window&0x000000FF)<< 24)); 

//     if (rotatedWindow == 0x000000012D) { //  for compressed point cloud (301)
//       Serial.print("301 ");
//       uint32_t size = 0x0;
//       // fill size in
//       for (int i = 0; i<4; i++) {
//         int b = Serial2.read();
//         size = (size << 8) | uint8_t(b);
//       } 
//     size = (((size&0xFF000000)>>24) | ((size&0x00FF0000)>>8) | ((size&0x0000FF00) <<8) | ((size&0x000000FF)<< 24));     
//     Serial.printf("len %d\n",size);

//       // fill XYZ unit size in

//       // THIS IS THE IDEAL FORMAT
//       uint32_t xyzUnit = 0x0;
//       int offSet = 0;
//       for (int i = 0; i<4; i++) {
//         int b = Serial2.read();
//         xyzUnit |= uint8_t(b)<<offSet;
//         offSet+=8;
//       } 
//     float xyzUnitFloat = 0;
//     memcpy(&xyzUnitFloat, &xyzUnit, sizeof(float));    
//     Serial.printf("xyzUnit %f\n",xyzUnitFloat); 

//   // fill in DopplerUnit
//         uint32_t dopplerUnit = 0x0;
//       for (int i = 0; i<4; i++) {
//         int b = Serial2.read();
//         dopplerUnit = (dopplerUnit << 8) | uint8_t(b);
//       } 
//     dopplerUnit = (((dopplerUnit&0xFF000000)>>24) | ((dopplerUnit&0x00FF0000)>>8) | ((dopplerUnit&0x0000FF00) <<8) | ((dopplerUnit&0x000000FF)<< 24));   
//       float dopplerUnitFloat = 0;
//     memcpy(&dopplerUnitFloat, &dopplerUnit, sizeof(float));    
//     Serial.printf("dopplerUnit %f\n",dopplerUnitFloat); 

//   // fill in SnrUnit
//         uint32_t SnrUnit = 0x0;
//       for (int i = 0; i<4; i++) {
//         int b = Serial2.read();
//         SnrUnit = (SnrUnit << 8) | uint8_t(b);
//       } 
//     SnrUnit = (((SnrUnit&0xFF000000)>>24) | ((SnrUnit&0x00FF0000)>>8) | ((SnrUnit&0x0000FF00) <<8) | ((SnrUnit&0x000000FF)<< 24));   
//       float SnrUnitFloat = 0;
//     memcpy(&SnrUnitFloat, &SnrUnit, sizeof(float));    
//     Serial.printf("SnrUnit %f\n",SnrUnitFloat); 

//   // fill in noiseUnit
//         uint32_t NoiseUnit = 0x0;
//       for (int i = 0; i<4; i++) {
//         int b = Serial2.read();
//         NoiseUnit = (NoiseUnit << 8) | uint8_t(b);
//       } 
//     NoiseUnit = (((NoiseUnit&0xFF000000)>>24) | ((NoiseUnit&0x00FF0000)>>8) | ((NoiseUnit&0x0000FF00) <<8) | ((NoiseUnit&0x000000FF)<< 24));   
//       float NoiseUnitFloat = 0;
//     memcpy(&NoiseUnitFloat, &NoiseUnit, sizeof(float));    
//     Serial.printf("noiseUnit %f\n",NoiseUnitFloat); 
//     }
//   }

//   // if (rotatedWindow == 0x0000012E) {
//   //   Serial.print("302");
//   // }
//   // if (rotatedWindow == 0x0000012F) {
//   //   Serial.print("303");
//   // }
//   //   if (rotatedWindow == 0x00000130) {
//   //   Serial.print("304");
//   // }
//   // if (rotatedWindow == 0x00000131) {
//   //   Serial.print("305");
//   // }
//   // if (rotatedWindow == 0x00000132) {
//   //   Serial.print("306");
//   // }
//   //   if (rotatedWindow == 0x00000133) {
//   //   Serial.print("307");
//   // }
//   // if (rotatedWindow == 0x00000134) {
//   //   Serial.print("308");
//   // }
//   // if (rotatedWindow == 0x00000135) {
//   //   Serial.print("309");
//   // }
//   //      Serial.printf("0x%04X\n", rotatedWindow);

// /// here is how to parse for uint16 
//   // or, if you really want to treat that as LE 0x0102:
//   // for 16 bits
//   // uint16_t le = (window >> 8) | (window << 8);
//   // if (le == 0x0102) {
//   //   Serial.println("Magic LE 0x0102 detected!");
//   // }
//   // if (le == 0x0304) {
//   //     Serial.println("Magic LE 0x0203 detected!");  
//   // }

 