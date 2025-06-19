/**
 *   @file  doaproc.h
 *
 *   @brief
 *      Implements Data path DoA processing functionality.
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
#ifndef DOAPROC_INTERNAL_H
#define DOAPROC_INTERNAL_H

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
#include <doaproc/v1/doaproc.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief
     *  HWA loop parameters
     *
     *  \ingroup DPU_DOAPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_DoaProc_hwaLoopCfg_t
    {
        /*! @brief  HWA number of loops */
        uint16_t hwaNumLoops;

        /*! @brief  HWA start paramset index */
        uint8_t hwaParamStartIdx;

        /*! @brief  HWA stop paramset index */
        uint8_t hwaParamStopIdx;
    } DPU_DoaProc_hwaLoopCfg;

    /**
     * @brief
     *  dopplerProc DPU internal data Object
     *
     * @details
     *  The structure is used to hold dopplerProc internal data object
     *
     *  \ingroup DPU_DOPPLERPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_DoaProc_Obj_t
    {
        /*! @brief HWA Handle */
        HWA_Handle hwaHandle;

        /*! @brief  EDMA driver handle. */
        EDMA_Handle edmaHandle;
        uint32_t    edmaInstanceId;

        /*! @brief  EDMA configuration for Input data (Radar cube -> HWA memory). */
        DPU_DoaProc_Edma edmaIn;

        /*! @brief  EDMA configuration for data output from HWA - Detection matrix */
        DPEDMA_ChanCfg edmaDetMatOut;

        /*! @brief  EDMA configuration for data in */
        DPEDMA_ChanCfg edmaInterLoopIn;

        /*! @brief HWA Processing Done semaphore Handle */
        SemaphoreP_Object hwaDoneSemaHandle;

        /*! @brief EDMA Done semaphore Handle */
        SemaphoreP_Object edmaDoneSemaHandle;

        /*! @brief Flag to indicate if DPU is in processing state */
        bool inProgress;

        /*! @brief  DMA trigger source channel for Ping param set */
        uint8_t hwaDmaTriggerSourceChan;

        /*! @brief  DMA trigger source channel for Ping param set */
        uint8_t hwaDmaTriggerSourcePing;

        /*! @brief  DMA trigger source channel for Pong param set */
        uint8_t hwaDmaTriggerSourcePong;


        /*! @brief  HWA number of loops */
        uint16_t hwaNumLoops;

        /*! @brief  HWA start paramset index */
        uint8_t hwaParamStartIdx;

        /*! @brief  HWA stop paramset index */
        uint8_t hwaParamStopIdx;

        /*! @brief  External range loop - HWA common config for the first processing part (doppler FFT and Azimuth FFT) */
        DPU_DoaProc_hwaLoopCfg hwaDopplerLoop;

        /*! @brief  External range loop - HWA common config for the second processing part (elevation FFT) */
        DPU_DoaProc_hwaLoopCfg hwaElevationLoop;

        /*! @brief  HWA memory bank addresses */
        uint32_t hwaMemBankAddr[DPU_DOAPROC_NUM_HWA_MEMBANKS];

        /*! @brief  Summation division shift for Doppler FFT non-coherent integration */
        uint8_t dopFftSumDiv;

        /*! @brief   Range loop type: 0 - HWA internal loop, 1- Loop controlled by CPU */
        uint16_t doaRangeLoopType;

        /*! @brief   Range Bias and rx channel gain/phase compensation configuration */
        DPU_DoaProc_compRxChannelBiasCfg compRxChanCfg;

        /*! @brief   Range Bias and rx channel gain/phase compensation enable flag */
        bool isRxChGainPhaseCompensationEnabled;

        /*! @brief  Number of virtual antennas */
        uint8_t numVirtualAntennas;

    } DPU_DoaProc_Obj;


#ifdef __cplusplus
}
#endif

#endif
