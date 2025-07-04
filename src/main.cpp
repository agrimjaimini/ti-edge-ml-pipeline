// // Prathik Narsetty

#include <Arduino.h>
#include "aws.h"
// //////////////////////////////////////////////////////////////////////////////
// /// helper functions -> need to move into a seperate file later ///
// //////////////////////////////////////////////////////////////////////////////

//  Read Byte
uint8_t readByte() {
    while (!Serial2.available());
    return (uint8_t)Serial2.read();
}

// Read 4 bytes MSB First
uint32_t readUint32BE() {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        v = (v << 8) | readByte();
    }
    // swap
    return ((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8) | ((v & 0x000000FF) << 24);
}

// Read a big endian float ( 4 bytes) and return float
float readFloatBE() {
    uint32_t raw = readUint32BE();
    float  f = 0;
    memcpy(&f, &raw, sizeof(f));
    return f;
}

// Read 2 bytes MSB first Unsigned 
uint16_t readUint16BE() {
    uint16_t v = 0;
    for (int i = 0; i < 2; i++) {
        v = (v << 8) | readByte();
    }
    return (v >> 8) | (v << 8);
}

// Read 2 bytes Signed
int16_t readInt16BE() {
    return (int16_t)readUint16BE();
}


/////////////////////////////////////////////
/// Setup /// 
/////////////////////////////////////////////
void setup() {
    Serial.begin(115200);
    Serial2.begin(1250000, SERIAL_8N1, D0, D1);
    connectAWS();  // if you need AWS
}

// global var
uint32_t window = 0;
uint16_t magicWord1 = 0;
uint32_t frameCounter = 0;
JsonDocument doc;
/////////////////////////////////////////////
/// loop ///
/////////////////////////////////////////////

void loop() {
  /*
  Need to use sliding window everytime im jumping data (to catch miss-allignment) makes our algo very robust
  -> can use fixed size everytime Im fitting the data exactly 
  */

// need to align per packet as I am going to start adding more data also need to do sliding window



   if (!Serial2.available()) return;

    int b = Serial2.read();
    if (b < 0) return;
    magicWord1 = (magicWord1 << 8) | uint8_t(b);

    // reconstruct
    uint16_t rotatedMagicWord1 = ((magicWord1 & 0xFF00) >>  8)  | ((magicWord1 & 0x00FF) <<  8);
// checks for all magic numbers
    if (rotatedMagicWord1 != 0x0102) {
      return;
    } 
    if (readUint16BE()!= 0x0304) {
      return; 
    }
    if (readUint16BE()!= 0x0506) {
      return; 
    }
    if (readUint16BE()!= 0x0708) {
      return; 
    }

    while (1) { // keep this going until I hit a another set of magic words and then exit as a fail safe

    // shift in one byte for sliding window of UART data
    if (!Serial2.available()) return;

    int b = Serial2.read();
    if (b < 0) return;

    window = (window << 8) | uint8_t(b);
    // check to see if we missed TLV and are at another packet (wont need this for the first TLV we parse otherwise we will skip a frame)
    uint16_t MagicWindow = window;
    if (((0xFF00&MagicWindow>>8) | ((0x00FF&MagicWindow)<<8)) == 0x0102) {
      if (readUint16BE()== 0x0304) {
        if (readUint16BE() == 0x0506) {
          if (readUint16BE() == 0x0708) {
            return;  // might need break
      }
    } 
  }       
    }
    // reconstruct
    uint32_t rotatedWindow = ((window & 0xFF000000) >> 24)  | ((window & 0x00FF0000) >>  8)  | ((window & 0x0000FF00) <<  8)  | ((window & 0x000000FF) << 24);

    if (rotatedWindow == 0x000000012D) {
        // Check for TLV 301 | compressed point cloud
        Serial.print("301 ");

        // length
        uint32_t size = readUint32BE();
        Serial.printf("len %u\n", size);

        // xyz Unit
        float xyzUnit = readFloatBE();
        Serial.printf("xyzUnit %f\n", xyzUnit);

        // doppler Unit
        float dopplerUnit = readFloatBE();
        Serial.printf("dopplerUnit %f\n", dopplerUnit);

        // Snr Unit 
        float snrUnit = readFloatBE();
        Serial.printf("snrUnit %f\n", snrUnit);
        // Noise Unit
        float noiseUnit = readFloatBE();
        Serial.printf("noiseUnit %f\n", noiseUnit);

        // 3) numDetectedPoints 
        uint16_t MajorPoints = readUint16BE();
        uint16_t MinorPoints = readUint16BE();
        uint16_t totalPoints = MajorPoints + MinorPoints;
        Serial.printf(
          "numDetectedPoints[0]=%u, [1]=%u, total=%u\n",
          MajorPoints, MinorPoints, totalPoints
        );

        // 4) loop through each point
        float x_pos [30] = {0};
        float y_pos [30] = {0};
        float z_pos [30] = {0};
        float Snr_arr [30] = {0};
        float Noise_arr [30] = {0};
        for (uint16_t i = 0; i < totalPoints && i<30; i++) {
            int16_t x = readInt16BE();
            int16_t y = readInt16BE();
            int16_t z = readInt16BE();
            int16_t doppler = readInt16BE();
            uint8_t pointSnr = readByte();
            uint8_t pointNoise = readByte();
            
            // fill up arrays
            x_pos[i] = x*xyzUnit;
            y_pos[i] = y*xyzUnit;
            z_pos[i] = z*xyzUnit;
            Snr_arr[i] = pointSnr*snrUnit;
            Noise_arr[i] = pointNoise*noiseUnit;

            Serial.printf(
              "Point %u: x=%d, y=%d, z=%d, snr=%u, noise=%u\n",
              i, x, y, z, pointSnr, pointNoise
            );
            int nPts = min((int)totalPoints, 30);
///////////////////////////////////////////////////////////
/// Let me set this section for presence detection ///
///////////////////////////////////////////////////////////

  // let me calculate a center of all these points
  float x_cen = 0;
  float y_cen = 0;
  float z_cen = 0;
  float snr_cen = 0;

  float x_sd = 0;
  float y_sd = 0;
  float z_sd = 0;
  float snr_sd = 0;
  // want to value points with high SNR 

        for (int i = 0; i<nPts; i++) {
          x_cen+= x_pos[i]/nPts;
          y_cen += y_pos[i]/nPts;
          z_cen += z_pos[i]/nPts;
          snr_cen += Snr_arr[i]/nPts;
        }

            // 2) compute sum of squared deviations
    float sum2_x = 0, sum2_y = 0, sum2_z = 0, sum2_snr;
    for (int i = 0; i < nPts; ++i) {
        float dx = x_pos[i] - x_cen;
        float dy = y_pos[i] - y_cen;
        float dz = z_pos[i] - z_cen;
        float dsnr = Snr_arr[i] - snr_cen;
        sum2_x += dx*dx;
        sum2_y += dy*dy;
        sum2_z += dz*dz;
        
    }

    // 3) compute SD
    // For a _population_ SD use nPts; for a _sample_ SD use (nPts-1)
    x_sd = sqrt(sum2_x /(nPts-1));
    y_sd = sqrt(sum2_y /(nPts-1));
    z_sd = sqrt(sum2_z /(nPts-1));






        }
                            // send data out via MQTT

              doc["Frame Count: "] = frameCounter;
              doc["Time Stamp: "] = millis();
              doc["Num Points: "] = totalPoints;

            JsonArray jx = doc["x_pos"].to<JsonArray>();
            JsonArray jy = doc["y_pos"].to<JsonArray>();
            JsonArray jz = doc["z_pos"].to<JsonArray>();
            JsonArray js = doc["snr"].to<JsonArray>();
            JsonArray jn = doc["noise"].to<JsonArray>();
            int nPts = min((int)totalPoints, 30);
            for (int i = 0; i < nPts; ++i) {
                jx.add(x_pos[i]);
                jy.add(y_pos[i]);
                jz.add(z_pos[i]);
                js.add(Snr_arr[i]);
                jn.add(Noise_arr[i]);
            }         
                char jsonBuffer[4096]; 
                serializeJson(doc, jsonBuffer); // print to client

                client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer); // only publishes the amount of the buffer which is used 
                Serial.print("hiii");
                Serial.println(jsonBuffer); 
                frameCounter++;
         // here is where presnece detection code goes
                     doc.clear();
        break;
    }
    
  }
 }
