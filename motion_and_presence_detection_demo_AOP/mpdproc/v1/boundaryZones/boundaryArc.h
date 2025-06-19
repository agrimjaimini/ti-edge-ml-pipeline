#ifndef BOUNDARY_ARC_H_
#define BOUNDARY_ARC_H_
#include <stdint.h>
#include <math.h>


typedef struct
{
    /**  @brief   Left boundary, m */
    float r1;
    /**  @brief   Right boundary, m */
    float r2;
    /**  @brief   Near boundary, m */
    float theta1;
    /**  @brief   Far boundary, m */
    float theta2;
    /**  @brief   Bottom boundary, m */
    float z1;
    /**  @brief   Top boundary, m */
    float z2;
} MPD_boundaryArc;


uint8_t zoneOccupancyFxnArc(float *input_point, void *zone_boundaries_ptr);

#endif
