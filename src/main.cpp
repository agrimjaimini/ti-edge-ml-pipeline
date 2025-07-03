// Prathik Narsetty

#include <Arduino.h>
#include "aws.h"
//////////////////////////////////////////////////////////////////////////////
/// helper functions -> need to move into a seperate file later ///
//////////////////////////////////////////////////////////////////////////////

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
    // connectAWS();  // if you need AWS
}

// global var
uint32_t window = 0;
/////////////////////////////////////////////
/// loop ///
/////////////////////////////////////////////
void loop() {
    // shift in one byte for sliding window of UART data
    if (!Serial2.available()) return;
    int b = Serial2.read();
    if (b < 0) return;
    window = (window << 8) | uint8_t(b);

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
        for (uint16_t i = 0; i < totalPoints; i++) {
            int16_t x = readInt16BE();
            int16_t y = readInt16BE();
            int16_t z = readInt16BE();
            int16_t doppler = readInt16BE();
            uint8_t pointSnr = readByte();
            uint8_t pointNoise = readByte();

            Serial.printf(
              "Point %u: x=%d, y=%d, z=%d, doppler=%d, snr=%u, noise=%u\n",
              i, x, y, z, doppler, pointSnr, pointNoise
            );
        }
    }
}
