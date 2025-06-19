#include "boundaryArc.h"


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
uint8_t zoneOccupancyFxnArc(float *input_point, void *zone_boundaries_ptr)
{
    float x = input_point[0];
    float y = input_point[1];

#ifndef CLUSTERING_2D
    float z = input_point[2];
#endif

#ifdef CLUSTERING_2D
    float r_meters = sqrt(pow(x, 2) + pow(y, 2));
#else
    float r_meters = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
#endif
    float theta_degrees = 180.0 / M_PI * atan(x / y);

    float rMin     = ((float *)zone_boundaries_ptr)[0];
    float rMax     = ((float *)zone_boundaries_ptr)[1];
    float thetaMin = ((float *)zone_boundaries_ptr)[2];
    float thetaMax = ((float *)zone_boundaries_ptr)[3];
#ifndef CLUSTERING_2D
    float zMin = ((float *)zone_boundaries_ptr)[4];
    float zMax = ((float *)zone_boundaries_ptr)[5];
#endif

#ifdef CLUSTERING_2D
    return (r_meters > rMin && r_meters <= rMax && theta_degrees > thetaMin && theta_degrees <= thetaMax);
#else
    return (r_meters > rMin && r_meters <= rMax && theta_degrees > thetaMin && theta_degrees <= thetaMax && z > zMin && z <= zMax);
#endif
}
