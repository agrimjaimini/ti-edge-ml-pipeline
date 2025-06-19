#include "boundaryZoneTemplate.h"


/* Template for the Zone Occupancy Function.
 * Requires :
 *
 * Input_point must be a float[2] of x, then y coordinates.
 * zone_boundaries_ptr must be a float[6], of xMin, xMax, yMin, yMax, zMin, Zmax coordinates
 *
 * Modifies :
 * None
 *
 * Effects : Returns whether the point is within the boundaries specified
 *   Computes whether an object is inside a rectangular box.
 *   NOTE - only calculates for x/y positions. z coordinates do not get taken into account
 *
 **/
uint8_t zoneOccupancyFxnBox(float *input_point, void *zone_boundaries_ptr)
{
    float x = input_point[0];
    float y = input_point[1];
#ifndef CLUSTERING_2D
    float z = input_point[2];
#endif

    float xMin = ((float *)zone_boundaries_ptr)[0];
    float xMax = ((float *)zone_boundaries_ptr)[1];
    float yMin = ((float *)zone_boundaries_ptr)[2];
    float yMax = ((float *)zone_boundaries_ptr)[3];
#ifndef CLUSTERING_2D
    float zMin = ((float *)zone_boundaries_ptr)[4];
    float zMax = ((float *)zone_boundaries_ptr)[5];
#endif

#ifdef CLUSTERING_2D
    return (x > xMin && x <= xMax && y > yMin && y <= yMax);
#else
    return (x > xMin && x <= xMax && y > yMin && y <= yMax && z > zMin && z <= zMax);
#endif
}
