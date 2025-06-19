#ifndef DBSCAN_H_
#define DBSCAN_H_

#include "define.h"
#include "mpdproc.h"

typedef struct DBscanOutput
{
    int indices[MAX_POINTS];
    int numPointsCluster[MAX_CLUSTERS];
    int numClusters;
#ifdef CLUSTERING_2D
    float clusterCentroids[2 * MAX_CLUSTERS];
#else
    float clusterCentroids[3 * MAX_CLUSTERS];
#endif
    float clusterSNR[MAX_CLUSTERS];
    float maxSNRinCluster[MAX_CLUSTERS];
} DBscanOutput;


float distances[MAX_POINTS * (MAX_POINTS + 1) / 2];

void dbscanOutputDataInit(DBscanOutput *dbscanOutputData);

void dbscan(DPIF_PointCloudCartesianExt *points, int numPoints, float epsilon, int16_t minPoints, DBscanOutput *dbscanOutputData);

#endif
