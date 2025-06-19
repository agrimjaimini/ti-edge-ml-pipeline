/**
 *   @file  cfarproccommon.h
 *
 *   @brief
 *      Implements Common definition across cfarcaProc DPU.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018-2022 Texas Instruments, Inc.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef CFAR_PROC_COMMON_H
#define CFAR_PROC_COMMON_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   Detection heatmap
 */
#define DPU_CFAR_RANGE_AZIMUTH_HEATMAP 0
#define DPU_CFAR_RANGE_DOPPLER_HEATMAP 1

/*! @brief   CFAR detection in range domain */
#define DPU_CFAR_RANGE_DOMAIN 0

/*! * @brief   CFAR detection in Doppler domain */
#define DPU_CFAR_DOPPLER_DOMAIN 1

/*! @brief Peak grouping scheme of CFAR detected objects based on peaks of neighboring cells taken from detection matrix */
#define DPU_CFAR_PEAK_GROUPING_DET_MATRIX_BASED 1

/*! @brief Peak grouping scheme of CFAR detected objects based only on peaks of neighboring cells that are already detected by CFAR */
#define DPU_CFAR_PEAK_GROUPING_CFAR_PEAK_BASED 2

/*! @brief  Convert log2 value in Q11 to 20log10 value
       Equation: output = (val / 2^11) * 20log10(2) = val * 2.939746e-03
 */
#define DPU_CFAR_CONV_LOG2Q11_TO_20LOG10(val) (val * 2.939746e-03)

/*
 * Minimum angle in the field of view is -90 degrees.
 * This is a safeguard to ensure that the minimum and maxiumum angles for range compensation aren't out of bounds.
 */
#define MIN_ANGLE_IN_FOV -90
/*
 * Minimum angle in the field of view is -90 degrees.
 * This is a safeguard to ensure that the minimum and maxiumum angles for range compensation aren't out of bounds.
 */
#define MAX_ANGLE_IN_FOV 90


    /**
     * @brief
     *  CFAR Configuration
     *
     * @details
     *  The structure contains the cfar configuration used in data path
     */
    typedef struct DPU_CFARProc_CfarCfg_t
    {
        /*! @brief    CFAR threshold scale */
        uint32_t thresholdScale;

        /*! @brief    CFAR threshold in dB*/
        float threshold_dB;

        /*! @brief    CFAR averagining mode 0-CFAR_CA, 1-CFAR_CAGO, 2-CFAR_CASO */
        uint8_t averageMode;

        /*! @brief    CFAR noise averaging one sided window length */
        uint8_t winLen;

        /*! @brief    CFAR one sided guard length*/
        uint8_t guardLen;

        /*! @brief    CFAR cumulative noise sum divisor
                      CFAR_CA:
                            noiseDivShift should account for both left and right noise window
                            ex: noiseDivShift = ceil(log2(2 * winLen))
                      CFAR_CAGO/_CASO:
                            noiseDivShift should account for only one sided noise window
                            ex: noiseDivShift = ceil(log2(winLen))
         */
        uint8_t noiseDivShift;

        /*! @brief    CFAR 0-cyclic mode disabled, 1-cyclic mode enabled */
        uint8_t cyclicMode;

        /*! @brief    Peak grouping scheme 1-based on neighboring peaks from detection matrix
         *                                 2-based on on neighboring CFAR detected peaks.
         *            Scheme 2 is not supported on the HWA version (cfarcaprochwa.h) */
        uint8_t peakGroupingScheme;

        /*! @brief     Peak grouping, 0- disabled, 1-enabled */
        uint8_t peakGroupingEn;

        /*! @brief  Side lobe threshold linear scale in Q8 format */
        int16_t sideLobeThresholdScaleQ8;

        /*! @brief  Check for peak being local peak in range direction */
        bool enableLocalMaxRange;

        /*! @brief  Check for peak being local peak in azimuth direction */
        bool enableLocalMaxAzimuth;

        /*! @brief  Interpolation in range direction */
        bool enableInterpRangeDom;

        /*! @brief  Interpolation in azimuth direction */
        bool enableInterpAzimuthDom;

        /*! @brief  Check for peak being local peak in Doppler domain (valid for Range/Doppler heatmap option) */
        bool enableLocalMaxDoppler;

        /*! @brief  Interpolation in Doppler domain (valid for Range/Doppler heatmap option) */
        bool enableInterpDopplerDom;

        /*! @brief  Interpolation in azimuth direction */
        bool lookUpTableCorrectAzimuthDom;

    } DPU_CFARProc_CfarCfg;

    /**
     * @brief
     *  CFAR Configuration
     *
     * @details
     *  The structure contains the cfar configuration used in data path
     */
    typedef struct DPU_CFARProc_CfarScndPassCfg_t
    {
        /*! @brief    CFAR threshold scale */
        uint32_t thresholdScale;

        /*! @brief    CFAR threshold in dB*/
        float threshold_dB;

        /*! @brief    CFAR averagining mode 0-CFAR_CA, 1-CFAR_CAGO, 2-CFAR_CASO */
        uint8_t averageMode;

        /*! @brief    CFAR noise averaging one sided window length */
        uint8_t winLen;

        /*! @brief    CFAR one sided guard length*/
        uint8_t guardLen;

        /*! @brief    CFAR cumulative noise sum divisor
                      CFAR_CA:
                            noiseDivShift should account for both left and right noise window
                            ex: noiseDivShift = ceil(log2(2 * winLen))
                      CFAR_CAGO/_CASO:
                            noiseDivShift should account for only one sided noise window
                            ex: noiseDivShift = ceil(log2(winLen))
         */
        uint8_t noiseDivShift;

        /*! @brief    CFAR 0-cyclic mode disabled, 1-cyclic mode enabled */
        uint8_t cyclicMode;

        /*! @brief     Peak grouping, 0- disabled, 1-enabled */
        uint8_t peakGroupingEn;

        /*! @brief     Second pass CFAR Enabled flag, 0- disabled, 1-enabled */
        uint8_t enabled;
    } DPU_CFARProc_CfarScndPassCfg;

    /**
     * @brief
     *  Data processing Unit statistics
     *
     * @details
     *  The structure is used to hold the statistics of the DPU
     *
     *  \ingroup INTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_CFARProc_Stats_t
    {
        /*! @brief total number of calls of DPU processing */
        uint32_t numProcess;

        /*! @brief total processing time during all chirps in a frame excluding EDMA waiting time*/
        uint32_t processingTime;

        /*! @brief total wait time for EDMA data transfer during all chirps in a frame*/
        uint32_t waitTime;
    } DPU_CFARProc_Stats;

    /**
     * @brief
     *  Field of view - AoA Configuration
     *
     * @details
     *  The structure contains the field of view - DoA configuration
     *
     *  \ingroup    DPU_CFARPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPU_CFARProc_AoaFovCfg_t
    {
        /*! @brief    minimum azimuth angle (in degrees) exported to Host*/
        float minAzimuthDeg;

        /*! @brief    maximum azimuth angle (in degrees) exported to Host*/
        float maxAzimuthDeg;

        /*! @brief    minimum elevation angle (in degrees) exported to Host*/
        float minElevationDeg;

        /*! @brief    maximum elevation angle (in degrees) exported to Host*/
        float maxElevationDeg;
    } DPU_CFARProc_AoaFovCfg;

    /**
     * @brief
     *  Field of view - Range Configuration
     *
     * @details
     *  The structure contains the field of view - DoA configuration
     *
     *  \ingroup    DPU_CFARPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPU_CFARProc_RangeFovCfg_t
    {
        /*! @brief    minimum Range */
        float min;

        /*! @brief    maximum Range */
        float max;

    } DPU_CFARProc_RangeFovCfg;

    /*!
     *  @brief    Holds number of samples to be skipped in detection process from
     *            left and right side of the dimension for range, azimuth and
     *            elevation dimension
     *
     *  \ingroup DPU_CFARPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPU_CFARProc_detectionCfg_t
    {
        /*! @brief    number of samples to be skipped from left in range dimension */
        int16_t skipLeftRange;
        /*! @brief    number of samples to be skipped from right in range dimension */
        int16_t skipRightRange;
        /*! @brief    number of samples to be skipped from left in azimuth dimension */
        int16_t skipLeftAzim;
        /*! @brief    number of samples to be skipped from right in azimuth dimension */
        int16_t skipRightAzim;
        /*! @brief    sample number where range compensation starts (from the left hand side) */
        int16_t rangeCompLeftStart1;
        /*! @brief    sample number where range compensation ends (from the right hand side) */
        int16_t rangeCompRightEnd1;
        /*! @brief    sample number where range compensation starts (from the left hand side) */
        int16_t rangeCompLeftStart2;
        /*! @brief    sample number where range compensation ends (from the right hand side) */
        int16_t rangeCompRightEnd2;
        /*! @brief    number of samples to be skipped from right in elevation dimension */
        int16_t secondaryCompSNRDrop;
        /*! @brief    number of samples to be skipped from left in elevation dimension */
        int16_t skipLeftElev;
        /*! @brief    number of samples to be skipped from right in elevation dimension */
        int16_t skipRightElev;
    } DPU_CFARProc_detectionCfg;

#ifdef __cplusplus
}
#endif

#endif
