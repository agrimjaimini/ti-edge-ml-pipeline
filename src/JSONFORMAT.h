// Prathik Narsetty
// This file will be exposed to the user to let them configure how they want to send the data over JSON 
#ifndef JSONFORMAT_H
#define JSONFORMAT_H
/*
Here is where you need to define your JSON format based on the incoming data. 
format it based of off what you are sending first across your end of UART
1) Change NUMPARAM to the number of parameters you are supposed to have
2)have a \ at the end of each line and make sure no space is after it.
3) Serial2.read reads 1 byte at a type. if your data is multiple bytes,
change the data type and read+shift the data in the order you sent it
4) change TIMEOUT_MS to how many ever MS you want to wait before a timeout
*/

#define NUMPARAM 6
#define TIMEOUT_MS 1000

#define FIELD_LIST                                   \
        do {                                         \
        uint8_t c = Serial2.read();                  \
        doc["c"] = c;                                \
        uint8_t d = Serial2.read();                  \
        doc["d"] = d;                                \
        uint8_t e = Serial2.read();                  \
        doc["e"] = e;                                \
        uint8_t f = Serial2.read();                  \
        doc["f"] = f;                                \
        uint8_t g = Serial2.read();                  \
        doc["g"] = g;                                \
        uint8_t h = Serial2.read();                  \
        doc["h"] = h;                                \
        } while (0)
    
        #endif