/**
 *   @file  aoasvcproc.h
 *
 *   @brief
 *      Implements Data path Aoasvc processing functionality.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2022 Texas Instruments, Inc.
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
#ifndef AOASVCPROC_INTERNAL_H
#define AOASVCPROC_INTERNAL_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */
#include <drivers/hwa.h>

/* DPIF Components Include Files */
#include <dpif/dpif_detmatrix.h>
#include <dpif/dpif_radarcube.h>

/* mmWave SDK Data Path Include Files */
#include <dpif/dp_error.h>
#include <aoasvcproc/v1/aoasvcproc.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief
     *  HWA loop parameters
     *
     *  \ingroup DPU_AOASVCPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_hwaLoopCfg_t
    {
        /*! @brief  HWA number of loops */
        uint16_t hwaNumLoops;

        /*! @brief  HWA start paramset index */
        uint8_t hwaParamStartIdx;

        /*! @brief  HWA stop paramset index */
        uint8_t hwaParamStopIdx;
    } DPU_AoasvcProc_hwaLoopCfg;

    typedef struct DPU_AoasvcProc_HwaParamCfg_t
    {
        /*! @brief  HWA start paramset index */
        uint8_t hwaParamStartIdx;

        /*! @brief  HWA stop paramset index */
        uint8_t hwaParamStopIdx;
    } DPU_AoasvcProc_HwaParamCfg;

    /**
     * @brief
     *  dopplerProc DPU internal data Object
     *
     * @details
     *  The structure is used to hold dopplerProc internal data object
     *
     *  \ingroup DPU_DOPPLERPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_Obj_t
    {
        /*! @brief HWA Handle */
        HWA_Handle hwaHandle;

        /*! @brief  EDMA driver handle. */
        EDMA_Handle edmaHandle;
        uint32_t    edmaInstanceId;

        /*! @brief  EDMA configuration for Input data (Radar cube -> HWA memory). */
        DPEDMA_ChanCfg edmaIn[2];

        /*! @brief  EDMA configuration for data output from HWA - Detection matrix */
        DPEDMA_ChanCfg edmaOut[2];

        /*! @brief  EDMA configuration for steering vectors transfer to HWA */
        DPEDMA_ChanCfg edmaInSteerVec;

        /*! @brief HWA Processing Done semaphore Handle */
        SemaphoreP_Object hwaDoneSemaHandle;

        /*! @brief EDMA Done semaphore Handle */
        SemaphoreP_Object edmaDoneSemaHandle;

        /*! @brief  Radar Cube 0-major, 1-minor */
        DPIF_RadarCube radarCube[2];

        /*! @brief  HWA memory bank addresses */
        uint32_t hwaMemBankAddr[DPU_AOASVCPROC_NUM_HWA_MEMBANKS];

        /*! @brief Flag to indicate if DPU is in processing state */
        bool inProgress;

        /*! @brief  HWA start paramset index */
        uint8_t hwaParamStartIdx;

        /*! @brief  HWA stop paramset index */
        uint8_t hwaParamStopIdx;

        DPU_AoasvcProc_HwaParamCfg dopplerHwaParamCfg;

        DPU_AoasvcProc_HwaParamCfg steerVecHwaParamCfg;

        /*! @brief  Summation division shift for Doppler FFT non-coherent integration */
        uint8_t dopFftSumDiv;

        /*! @brief   Range loop type: 0 - HWA internal loop, 1- Loop controlled by CPU */
        uint16_t aoasvcRangeLoopType;

        /*! @brief   Number of Doppler bins */
        uint16_t numDopplerBins;

        /*! @brief   Number of virtual antennas */
        uint16_t numVirtualAntennas;

        float   rangeStep;
        float   azimStart;
        float   azimStep;
        float   elevStart;
        float   elevStep;
        int16_t numAzimVec;
        int16_t numElevVec;
        bool    enableSteeringVectorCorrection;
        bool    enableAngleInterpolation;

    } DPU_AoasvcProc_Obj;


#ifdef __cplusplus
}
#endif

#endif
