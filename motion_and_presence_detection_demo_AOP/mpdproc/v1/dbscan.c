#include "dbscan.h"
#include <math.h>

void dbscanOutputDataInit(DBscanOutput *dbscanOutputData)
{
    int i;
    for (i = 0; i < MAX_POINTS; i++)
        dbscanOutputData->indices[i] = 0;
    for (i = 0; i < MAX_CLUSTERS; i++)
    {
        dbscanOutputData->numPointsCluster[i] = 0;
#ifdef CLUSTERING_2D
        dbscanOutputData->clusterCentroids[2 * i]     = 0;
        dbscanOutputData->clusterCentroids[2 * i + 1] = 0;
#else
        dbscanOutputData->clusterCentroids[3 * i]     = 0;
        dbscanOutputData->clusterCentroids[3 * i + 1] = 0;
        dbscanOutputData->clusterCentroids[3 * i + 2] = 0;
#endif
        dbscanOutputData->clusterSNR[i]      = 0;
        dbscanOutputData->maxSNRinCluster[i] = 0;
    }
    dbscanOutputData->numClusters = 0;
}

int findNeighbours(int i, float epsilon, int *neighbours, int numPoints)
{
    int j, numNeighbours = 0;
    for (j = 0; j < numPoints; j++)
    {
        if (i != j)
        {
            float distance;
            if (i > j)
                distance = distances[(i - 1) * i / 2 + j];
            else
                distance = distances[(j - 1) * j / 2 + i];
            if (distance <= epsilon)
                neighbours[numNeighbours++] = j;
        }
    }
    neighbours[numNeighbours++] = i;
    return numNeighbours;
}

void expandCluster(int point, int *neighbours, int numNeighbours, int clusterID, float epsilon, int16_t minPoints, int *visited, int numPoints, DBscanOutput *dbscanOutputData, DPIF_PointCloudCartesianExt *points)
{
    dbscanOutputData->indices[point]                  = clusterID;
    dbscanOutputData->numPointsCluster[clusterID - 1] = 1;
#ifdef CLUSTERING_2D
    dbscanOutputData->clusterCentroids[2 * (clusterID - 1)] += points[point].x;
    dbscanOutputData->clusterCentroids[2 * (clusterID - 1) + 1] += points[point].y;
#else
    dbscanOutputData->clusterCentroids[3 * (clusterID - 1)] += points[point].x;
    dbscanOutputData->clusterCentroids[3 * (clusterID - 1) + 1] += points[point].y;
    dbscanOutputData->clusterCentroids[3 * (clusterID - 1) + 2] += points[point].z;
#endif
    dbscanOutputData->clusterSNR[clusterID - 1] += points[point].snr;
    dbscanOutputData->maxSNRinCluster[clusterID - 1] = points[point].snr;
    int k                                            = 0;
    while (1)
    {
        int j = neighbours[k];
        if (!visited[j])
        {
            visited[j] = 1;
            int newNeighbours[MAX_POINTS];
            int numNewNeighbours = findNeighbours(j, epsilon, newNeighbours, numPoints);
            if (numNewNeighbours >= minPoints)
            {
                int i = 0, j = numNeighbours;
                for (i = 0; i < numNewNeighbours; i++)
                {
                    if (newNeighbours[i] == k)
                        continue;
                    int l, flag = 0;
                    for (l = 0; l < numNeighbours; l++)
                    {
                        if (newNeighbours[i] == neighbours[l])
                        {
                            flag = 1;
                            break;
                        }
                    }
                    if (!flag)
                        neighbours[j++] = newNeighbours[i];
                }
                numNeighbours = j;
            }
        }
        if (dbscanOutputData->indices[j] == 0)
        {
            dbscanOutputData->indices[j] = clusterID;
            dbscanOutputData->numPointsCluster[clusterID - 1]++;
#ifdef CLUSTERING_2D
            dbscanOutputData->clusterCentroids[2 * (clusterID - 1)] += points[j].x;
            dbscanOutputData->clusterCentroids[2 * (clusterID - 1) + 1] += points[j].y;
#else
            dbscanOutputData->clusterCentroids[3 * (clusterID - 1)] += points[j].x;
            dbscanOutputData->clusterCentroids[3 * (clusterID - 1) + 1] += points[j].y;
            dbscanOutputData->clusterCentroids[3 * (clusterID - 1) + 2] += points[j].z;
#endif
            dbscanOutputData->clusterSNR[clusterID - 1] += points[j].snr;
            dbscanOutputData->maxSNRinCluster[clusterID - 1] = MAX(points[j].snr, dbscanOutputData->maxSNRinCluster[clusterID - 1]);
        }
        k++;
        if (k >= numNeighbours)
            break;
    }
}

void calculateDistances(DPIF_PointCloudCartesianExt *points, int numPoints)
{
    int i, j;
    for (i = 0; i < numPoints; i++)
    {
        for (j = 0; j < i; j++)
        {
#ifdef CLUSTERING_2D
            distances[(i - 1) * i / 2 + j] = (float)sqrt(pow(points[i].x - points[j].x, 2) + pow(points[i].y - points[j].y, 2));
#else
            distances[(i - 1) * i / 2 + j] = (float)sqrt(pow(points[i].x - points[j].x, 2) + pow(points[i].y - points[j].y, 2) + pow(points[i].z - points[j].z, 2));
#endif
        }
    }
}

void dbscan(DPIF_PointCloudCartesianExt *points, int numPoints, float epsilon, int16_t minPoints, DBscanOutput *dbscanOutputData)
{
    int clusterID           = 0;
    int visited[MAX_POINTS] = { 0 };
    int isnoise[MAX_POINTS] = { 0 };
    int point;
    int i;

    calculateDistances(points, numPoints);

    for (point = 0; point < numPoints; point++)
    {
        if (!visited[point])
        {
            visited[point] = 1;
            int neighbours[MAX_POINTS];
            int numNeighbours = findNeighbours(point, epsilon, neighbours, numPoints);

            if (numNeighbours < minPoints)
                isnoise[point] = 1;
            else
                expandCluster(point, neighbours, numNeighbours, ++clusterID, epsilon, minPoints, visited, numPoints, dbscanOutputData, points);
        }
    }
    dbscanOutputData->numClusters = clusterID;
    for (i = 0; i < dbscanOutputData->numClusters; i++)
    {
#ifdef CLUSTERING_2D
        dbscanOutputData->clusterCentroids[2 * i] /= dbscanOutputData->numPointsCluster[i];
        dbscanOutputData->clusterCentroids[2 * i + 1] /= dbscanOutputData->numPointsCluster[i];
#else
        dbscanOutputData->clusterCentroids[3 * i] /= dbscanOutputData->numPointsCluster[i];
        dbscanOutputData->clusterCentroids[3 * i + 1] /= dbscanOutputData->numPointsCluster[i];
        dbscanOutputData->clusterCentroids[3 * i + 2] /= dbscanOutputData->numPointsCluster[i];
#endif
    }
}
