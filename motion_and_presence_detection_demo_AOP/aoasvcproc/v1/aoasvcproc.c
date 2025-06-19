/**
 *   @file  aoasvcproc.c
 *
 *   @brief
 *      Implements Data path range gate processing Unit using HWA.
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

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK driver/common Include Files */
#include <common/syscommon.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/HeapP.h>
#include <drivers/edma.h>
#include <drivers/soc.h>
#include <drivers/hw_include/hw_types.h>


/* Utils */
#include <utils/mathutils/mathutils.h>

/* Data Path Include files */
#include <dpedma/v1/dpedmahwa.h>
#include <aoasvcproc/v1/aoasvcproc.h>
#include <aoasvcproc/v1/aoasvcprocinternal.h>
#include <dpif/dpif_pointcloud.h>

#include <aoasvcproc/v1/aoasv_table.h>

/* Flag to check input parameters */
#define DEBUG_CHECK_PARAMS 1

#define AOASVC_PROFILE_ENABLE 1

#define DPU_AOASVCPROC_BANK_0 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(CSL_APP_HWA_DMA0_RAM_BANK0_BASE)
#define DPU_AOASVCPROC_BANK_1 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(CSL_APP_HWA_DMA0_RAM_BANK1_BASE)
#define DPU_AOASVCPROC_BANK_2 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(CSL_APP_HWA_DMA0_RAM_BANK2_BASE)
#define DPU_AOASVCPROC_BANK_3 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(CSL_APP_HWA_DMA0_RAM_BANK3_BASE)

/**
 * @brief   Detection matrix data format ToDo should be defined in SDK
 */
#define DPU_AOASVCPROC_DPIF_DETMATRIX_FORMAT_2 2

/* User defined heap memory and handle */
#define AOASVCPROC_HEAP_MEM_SIZE (sizeof(DPU_AoasvcProc_Obj))

static uint8_t gAoasvcProcHeapMem[AOASVCPROC_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

//#include <aoasvcproc/v0/aoasvc_ideal_az81_3_el30_3.h>
//#include <datapath/dpu/aoasvcproc/v0/aoasvc_ideal_az80_2p5_el60_5.h>

/*===========================================================
 *                    Internal Functions
 *===========================================================*/
// The function reads the FRAME_REF_TIMER that runs free at 40MHz
extern uint32_t Cycleprofiler_getTimeStamp(void);

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function.
 *  \ingroup    DPU_AOASVC_INTERNAL_FUNCTION
 */
static void aoasvcProc_hwaDoneIsrCallback(void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/**
 *  @b Description
 *  @n
 *      EDMA completion call back function.
 *
 *  @param[in] intrHandle    EDMA Interrupt handle
 *  @param[in] arg           Input argument is pointer to semaphore object
 *
 *  \ingroup    DPU_AOASVC_INTERNAL_FUNCTION
 */
static void aoasvcProc_edmaDoneIsrCallback(Edma_IntrHandle intrHandle, void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/**
 *  @b Description
 *  @n
 *      Configures HWA for Doppler processing.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_AOASVC_INTERNAL_FUNCTION
 *
 *  @retval error code.
 */
static inline int32_t aoasvcProc_configHwa(
    DPU_AoasvcProc_Obj    *obj,
    DPU_AoasvcProc_Config *cfg)
{
    HWA_ParamConfig     hwaParamCfg;
    HWA_InterruptConfig paramISRConfig;
    uint32_t            paramsetIdx = 0;
    int32_t             retVal      = 0U;
    uint32_t            idx;
    uint8_t             destChan;

    /* Currently no scaling in Doppler FFT. */
    obj->dopFftSumDiv = 0; // mathUtils_ceilLog2(cfg->staticCfg.numDopplerBins);

    obj->hwaParamStartIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    ;
    obj->dopplerHwaParamCfg.hwaParamStartIdx = obj->hwaParamStartIdx;

    paramsetIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    /********************************************************************************/
    /*                    Doppler FFT Configuration                                 */
    /********************************************************************************/
    for (idx = 0; idx < 2; idx++)
    {
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
        hwaParamCfg.dmaTriggerSrc = cfg->hwRes.hwaCfg.dmaTrigSrcChan[idx];

        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = DPU_AOASVCPROC_BANK_0 + idx * CSL_APP_HWA_BANK_SIZE;
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numDopplerChirps - 1; // size in samples - 1

        hwaParamCfg.source.srcAIdx          = cfg->staticCfg.numVirtualAntennas * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = cfg->staticCfg.numVirtualAntennas - 1;
        hwaParamCfg.source.srcBIdx          = sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0; // no conjugate
        hwaParamCfg.source.srcScale         = 8;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_AOASVCPROC_BANK_2 + idx * CSL_APP_HWA_BANK_SIZE;
        hwaParamCfg.dest.dstAcnt        = (cfg->staticCfg.numDopplerBins) - 1;
        hwaParamCfg.dest.dstAIdx        = cfg->staticCfg.numVirtualAntennas * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstBIdx        = sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;
        hwaParamCfg.dest.dstSkipInit    = 0; // no skip even when CLUTTER RMOVAL enabled

        hwaParamCfg.accelModeArgs.fftMode.fftEn   = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = cfg->staticCfg.log2NumDopplerBins;

        /* scaling is disabled in all stages */
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling   = (cfg->staticCfg.numDopplerBins - 1) >> 5;
        hwaParamCfg.accelModeArgs.fftMode.interfZeroOutEn    = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowEn           = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowStart        = 0;
        hwaParamCfg.accelModeArgs.fftMode.winSymm            = 0;
        hwaParamCfg.accelModeArgs.fftMode.winInterpolateMode = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn           = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode         = HWA_FFT_MODE_OUTPUT_DEFAULT;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;

        retVal = HWA_configParamSet(obj->hwaHandle,
                                    paramsetIdx,
                                    &hwaParamCfg,
                                    NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Make sure DMA interrupt/trigger is disabled for this paramset*/
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                     cfg->hwRes.edmaCfg.edmaOut[idx].channel,
                                     &destChan);
        if (retVal != 0)
        {
            goto exit;
        }
        /* Now enable interrupt */
        paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
        paramISRConfig.dma.dstChannel    = destChan;
        paramISRConfig.cpu.callbackArg   = NULL;
        retVal                           = HWA_enableParamSetInterrupt(obj->hwaHandle, paramsetIdx, &paramISRConfig);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;
    }
    obj->dopplerHwaParamCfg.hwaParamStopIdx = paramsetIdx - 1;
    obj->hwaParamStopIdx                    = obj->dopplerHwaParamCfg.hwaParamStopIdx;

    if (cfg->staticCfg.enableSteeringVectorCorrection)
    {
        obj->steerVecHwaParamCfg.hwaParamStartIdx = paramsetIdx;
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_SOFTWARE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = DPU_AOASVCPROC_BANK_0;
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numVirtualAntennas - 1; // size in samples - 1

        hwaParamCfg.source.srcAIdx          = sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = cfg->staticCfg.numAzimVec * cfg->staticCfg.numElevVec - 1;
        hwaParamCfg.source.srcBIdx          = cfg->staticCfg.numVirtualAntennas * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0; // Do not conjugate! Steering vectors are saved as exp(-1j...), re0,im0,re1,im1,...
        hwaParamCfg.source.srcScale         = 4;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_AOASVCPROC_BANK_3;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;


        hwaParamCfg.accelModeArgs.fftMode.fftEn      = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize    = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn   = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_SUM_STATS;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_VECTOR_MULT;

        retVal = HWA_configParamSet(obj->hwaHandle,
                                    paramsetIdx,
                                    &hwaParamCfg,
                                    NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Make sure DMA interrupt/trigger is disabled for this paramset*/
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;

        /* Search for maximum */
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = DPU_AOASVCPROC_BANK_3;
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numAzimVec * cfg->staticCfg.numElevVec - 1; // size in samples - 1

        hwaParamCfg.source.srcAIdx          = sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBcnt          = 1 - 1;
        hwaParamCfg.source.srcBIdx          = 0;
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0;
        hwaParamCfg.source.srcScale         = 0;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_AOASVCPROC_BANK_2 + CSL_APP_HWA_BANK_SIZE - sizeof(DPU_AoasvcProc_HwaMaxOutput);
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;


        hwaParamCfg.accelModeArgs.fftMode.fftEn      = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize    = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn   = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_MAX_STATS;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;

        retVal = HWA_configParamSet(obj->hwaHandle,
                                    paramsetIdx,
                                    &hwaParamCfg,
                                    NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Make sure DMA interrupt/trigger is disabled for this paramset*/
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;

        obj->steerVecHwaParamCfg.hwaParamStopIdx = paramsetIdx - 1;
        obj->hwaParamStopIdx                     = obj->steerVecHwaParamCfg.hwaParamStopIdx;
    }

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *  EDMA configuration.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_AOASVC_INTERNAL_FUNCTION
 *
 *  @retval EDMA error code, see EDMA API.
 */
static inline int32_t aoasvcProc_configEdma(
    DPU_AoasvcProc_Obj    *obj,
    DPU_AoasvcProc_Config *cfg)
{
    int32_t            retVal = SystemP_SUCCESS;
    DPEDMA_ChainingCfg chainingCfg;
    DPEDMA_syncABCfg   syncABCfg;

    bool               isTransferCompletionEnabled;
    bool               isIntermediateTransferInterruptEnabled;
    Edma_EventCallback transferCompletionCallbackFxn;
    void              *transferCompletionCallbackFxnArg;
    uint8_t            transferType;
    uint32_t           pingPongIdx;

    if (obj == NULL)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }


    /*****************************************************************************************/
    /**************                     PROGRAM DMA INPUT                    *****************/
    /*****************************************************************************************/
    for (pingPongIdx = 0; pingPongIdx < 2; pingPongIdx++)
    {
        chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaHotSig.channel;
        chainingCfg.isIntermediateChainingEnabled = true;
        chainingCfg.isFinalChainingEnabled        = true;


        syncABCfg.srcAddress  = (uint32_t)(cfg->hwRes.radarCube[0].data); // To be filled per point cloud point
        syncABCfg.destAddress = (uint32_t)obj->hwaMemBankAddr[0] + pingPongIdx * CSL_APP_HWA_BANK_SIZE;

        syncABCfg.aCount  = sizeof(cmplx16ImRe_t);
        syncABCfg.bCount  = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * cfg->staticCfg.numDopplerChirps;
        syncABCfg.cCount  = 1;
        syncABCfg.srcBIdx = cfg->staticCfg.numRangeBins * sizeof(cmplx16ImRe_t);
        syncABCfg.srcCIdx = 0;
        syncABCfg.dstBIdx = sizeof(cmplx16ImRe_t);
        syncABCfg.dstCIdx = 0;

        retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                     &cfg->hwRes.edmaCfg.edmaIn[pingPongIdx],
                                     &chainingCfg,
                                     &syncABCfg,
                                     false, // isEventTriggered
                                     false, // isIntermediateTransferCompletionEnabled
                                     false, // isTransferCompletionEnabled
                                     NULL, // transferCompletionCallbackFxn
                                     NULL, // transferCompletionCallbackFxnArg
                                     NULL);

        if (retVal != SystemP_SUCCESS)
        {
            goto exit;
        }
    }

    /******************************************************************************************
     *  PROGRAM DMA Hot Signature
     ******************************************************************************************/
    retVal = DPEDMAHWA_configTwoHotSignature(cfg->hwRes.edmaCfg.edmaHandle,
                                             &cfg->hwRes.edmaCfg.edmaHotSig,
                                             obj->hwaHandle,
                                             cfg->hwRes.hwaCfg.dmaTrigSrcChan[0],
                                             cfg->hwRes.hwaCfg.dmaTrigSrcChan[1],
                                             false);

    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************/
    /**************                      PROGRAM DMA OUTPUT                   *****************/
    /******************************************************************************************/


    for (pingPongIdx = 0; pingPongIdx < 2; pingPongIdx++)
    {
        transferType                              = 1;
        chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaOut[pingPongIdx].channel;
        chainingCfg.isIntermediateChainingEnabled = false;
        chainingCfg.isFinalChainingEnabled        = false;

#if 0 // EDMA_INT
        isIntermediateTransferInterruptEnabled = true;
        isTransferCompletionEnabled = true;
        transferCompletionCallbackFxn = aoasvcProc_edmaDoneIsrCallback;
        transferCompletionCallbackFxnArg = (void *)&(obj->edmaDoneSemaHandle);
#else
        isIntermediateTransferInterruptEnabled = true;
        isTransferCompletionEnabled            = true;
        transferCompletionCallbackFxn          = NULL;
        transferCompletionCallbackFxnArg       = NULL;
#endif

        syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[2] + pingPongIdx * CSL_APP_HWA_BANK_SIZE;
        syncABCfg.destAddress = (uint32_t)cfg->hwRes.virtAntElemList + 2 * sizeof(uint16_t) + pingPongIdx * sizeof(DPU_AoasvcProc_VirtualAntennaElements); // skip range and doppler fields
        syncABCfg.aCount      = sizeof(cmplx32ImRe_t) * obj->numVirtualAntennas;
        syncABCfg.bCount      = 1;
        syncABCfg.cCount      = 2; // To be set per frame to (numberOfPoints+1) >> 1  (make total even number and divide by 2 because of ping/pong split
        syncABCfg.srcBIdx     = 0;
        syncABCfg.dstBIdx     = 0;
        syncABCfg.dstCIdx     = 2 * sizeof(DPU_AoasvcProc_VirtualAntennaElements);
        syncABCfg.srcCIdx     = 0;

        retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                           &cfg->hwRes.edmaCfg.edmaOut[pingPongIdx],
                                           &chainingCfg,
                                           &syncABCfg,
                                           true, // isEventTriggered
                                           isIntermediateTransferInterruptEnabled,
                                           isTransferCompletionEnabled,
                                           transferCompletionCallbackFxn,
                                           transferCompletionCallbackFxnArg,
                                           &cfg->hwRes.edmaCfg.intrObj[pingPongIdx],
                                           transferType);
        if (retVal != SystemP_SUCCESS)
        {
            goto exit;
        }
    }

    if (cfg->staticCfg.enableSteeringVectorCorrection)
    {
        transferType                              = 1;
        chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaInSteerVec.channel;
        chainingCfg.isIntermediateChainingEnabled = false;
        chainingCfg.isFinalChainingEnabled        = false;

        isIntermediateTransferInterruptEnabled = true;
        isTransferCompletionEnabled            = true;
        transferCompletionCallbackFxn          = NULL;
        transferCompletionCallbackFxnArg       = NULL;


        // memcpy((void *) obj->hwaMemBankAddr[0], (void *)SteeringVectors, (int32_t)obj->numAzimVec * (int32_t)obj->numElevVec * (int32_t)obj->numVirtualAntennas * sizeof(cmplx16ImRe_t));

        syncABCfg.srcAddress  = (uint32_t)SteeringVectors;
        syncABCfg.destAddress = (uint32_t)obj->hwaMemBankAddr[0];
        syncABCfg.aCount      = (int32_t)obj->numAzimVec * (int32_t)obj->numElevVec * (int32_t)obj->numVirtualAntennas * sizeof(cmplx16ImRe_t);
        syncABCfg.bCount      = 1;
        syncABCfg.cCount      = 1;
        syncABCfg.srcBIdx     = 0;
        syncABCfg.dstBIdx     = 0;
        syncABCfg.dstCIdx     = 0;
        syncABCfg.srcCIdx     = 0;

        retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                           &cfg->hwRes.edmaCfg.edmaInSteerVec,
                                           &chainingCfg,
                                           &syncABCfg,
                                           false, // isEventTriggered
                                           isIntermediateTransferInterruptEnabled,
                                           isTransferCompletionEnabled,
                                           transferCompletionCallbackFxn,
                                           transferCompletionCallbackFxnArg,
                                           &cfg->hwRes.edmaCfg.intrObjSteerVecTransfer,
                                           transferType);
        if (retVal != SystemP_SUCCESS)
        {
            goto exit;
        }
    }
exit:
    return (retVal);
}

/*===========================================================
 *                    Doppler Proc External APIs
 *===========================================================*/

/**
 *  @b Description
 *  @n
 *      dopplerProc DPU init function. It allocates memory to store
 *  its internal data object and returns a handle if it executes successfully.
 *
 *  @param[in]   initCfg Pointer to initial configuration parameters
 *  @param[out]  errCode Pointer to errCode generates by the API
 *
 *  \ingroup    DPU_AOASVCPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid handle
 *  @retval
 *      Error       - NULL
 */
DPU_AoasvcProc_Handle DPU_AoasvcProc_init(
    DPU_AoasvcProc_InitParams *initCfg,
    int32_t                   *errCode)
{
    DPU_AoasvcProc_Obj *obj = NULL;
    HWA_MemInfo         hwaMemInfo;
    uint32_t            i;
    int32_t             status = SystemP_SUCCESS;

    *errCode = 0;

    if ((initCfg == NULL) || (initCfg->hwaHandle == NULL))
    {
        *errCode = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }

    /* Allocate memory */
    obj = (DPU_AoasvcProc_Obj *)&gAoasvcProcHeapMem;
    if (obj == NULL)
    {
        *errCode = DPU_AOASVCPROC_ENOMEM;
        goto exit;
    }

    /* Initialize memory */
    memset((void *)obj, 0U, sizeof(DPU_AoasvcProc_Obj));

    // RPMF printf("AOASVC DPU: (DPU_AoasvcProc_Obj *) 0x%08x\n", (uint32_t) obj);

    /* Save init config params */
    obj->hwaHandle = initCfg->hwaHandle;

    /* Create DPU semaphores */
    status = SemaphoreP_constructCounting(&obj->edmaDoneSemaHandle, 0, 10);
    // status = SemaphoreP_constructBinary(&obj->edmaDoneSemaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_AOASVCPROC_ESEMA;
        goto exit;
    }

    status = SemaphoreP_constructBinary(&obj->hwaDoneSemaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_AOASVCPROC_ESEMA;
        goto exit;
    }

    /* Populate HWA base addresses and offsets. This is done only once, at init time.*/
    *errCode = HWA_getHWAMemInfo(obj->hwaHandle, &hwaMemInfo);
    if (*errCode < 0)
    {
        goto exit;
    }

    /* check if we have enough memory banks*/
    if (hwaMemInfo.numBanks < DPU_AOASVCPROC_NUM_HWA_MEMBANKS)
    {
        *errCode = DPU_AOASVCPROC_EHWARES;
        goto exit;
    }

    for (i = 0; i < DPU_AOASVCPROC_NUM_HWA_MEMBANKS; i++)
    {
        obj->hwaMemBankAddr[i] = hwaMemInfo.baseAddress + i * hwaMemInfo.bankSize;
    }

exit:

    if (*errCode < 0)
    {
        if (obj != NULL)
        {
            obj = NULL;
        }
    }
    return ((DPU_AoasvcProc_Handle)obj);
}

/**
 *  @b Description
 *  @n
 *   AOASVC DPU configuration
 *
 *  @param[in]   handle     DPU handle.
 *  @param[in]   cfg        Pointer to configuration parameters.
 *
 *  \ingroup    DPU_AOASVCPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success      = 0
 *  @retval
 *      Error       != 0 @ref DPU_DOPPLERPROC_ERROR_CODE
 */
int32_t DPU_AoasvcProc_config(
    DPU_AoasvcProc_Handle  handle,
    DPU_AoasvcProc_Config *cfg)
{
    DPU_AoasvcProc_Obj *obj;
    int32_t             retVal = 0;


    obj = (DPU_AoasvcProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }


#if DEBUG_CHECK_PARAMS
    /* Validate params */
    if (!cfg ||
        !cfg->hwRes.edmaCfg.edmaHandle)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }

    /* Check if radar cube format is supported by DPU*/
    if ((cfg->hwRes.radarCube[0].datafmt != DPIF_RADARCUBE_FORMAT_6) || (cfg->hwRes.radarCube[1].datafmt != DPIF_RADARCUBE_FORMAT_6))
    {
        retVal = DPU_AOASVCPROC_ECUBEFORMAT;
        goto exit;
    }

#endif

    /* Check if the steering vectors can fit into three HWA memory banks minus 8 bytes (space for the final result: the peak index) */
    if (cfg->staticCfg.enableSteeringVectorCorrection)
    {
        /* Check if steering vectors can fit in 3 HWA memory banks */
        int32_t sterrVecSizeBytes = cfg->staticCfg.numAzimVec * cfg->staticCfg.numElevVec * cfg->staticCfg.numVirtualAntennas * sizeof(cmplx16ImRe_t);
        if (sterrVecSizeBytes > (3 * CSL_APP_HWA_BANK_SIZE - sizeof(DPU_AoasvcProc_HwaMaxOutput)))
        {
            retVal = DPU_AOASVCPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }

    /* Save necessary parameters to DPU object that will be used during Process time */
    /* EDMA parameters needed to trigger first EDMA transfer*/
    obj->edmaHandle = cfg->hwRes.edmaCfg.edmaHandle;
    memcpy((void *)(obj->edmaIn), (void *)(cfg->hwRes.edmaCfg.edmaIn), 2 * sizeof(DPEDMA_ChanCfg));
    memcpy((void *)(obj->edmaOut), (void *)(cfg->hwRes.edmaCfg.edmaOut), 2 * sizeof(DPEDMA_ChanCfg));
    memcpy((void *)(&obj->edmaInSteerVec), (void *)(&cfg->hwRes.edmaCfg.edmaInSteerVec), sizeof(DPEDMA_ChanCfg));


    /*! @brief  Radar Cube */
    memcpy(obj->radarCube, cfg->hwRes.radarCube, 2 * sizeof(DPIF_RadarCube));

    obj->numDopplerBins                 = cfg->staticCfg.numDopplerBins;
    obj->numVirtualAntennas             = cfg->staticCfg.numVirtualAntennas;
    obj->enableSteeringVectorCorrection = cfg->staticCfg.enableSteeringVectorCorrection;
    obj->enableAngleInterpolation       = cfg->staticCfg.enableAngleInterpolation;
    obj->rangeStep                      = cfg->staticCfg.rangeStep;
    obj->azimStart                      = cfg->staticCfg.azimStart * 3.14159265 / 180; // Convert degrees to radians
    obj->azimStep                       = cfg->staticCfg.azimStep * 3.14159265 / 180;
    obj->elevStart                      = cfg->staticCfg.elevStart * 3.14159265 / 180;
    obj->elevStep                       = cfg->staticCfg.elevStep * 3.14159265 / 180;
    obj->numAzimVec                     = cfg->staticCfg.numAzimVec;
    obj->numElevVec                     = cfg->staticCfg.numElevVec;

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }


    /*******************************/
    /**  Configure HWA            **/
    /*******************************/
    retVal = aoasvcProc_configHwa(obj, cfg);
    if (retVal != 0)
    {
        goto exit;
    }

    /*******************************/
    /**  Configure EDMA           **/
    /*******************************/
    retVal = aoasvcProc_configEdma(obj, cfg);
    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n Configures Input EDMA
 *
 *  @param[in]   obj             DPU internal object
 *  @param[in]   radarCubeSrc  Structure descriptor of the input radar cube
 *
 *  \ingroup    DPU_AOASVCPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t aoasvcProc_setInputOutputEdma(DPU_AoasvcProc_Obj                    *obj,
                                      uint32_t                               idx,
                                      DPIF_PointCloudRngAzimElevDopInd      *detObjIndOut,
                                      uint32_t                               numPointsMajor,
                                      DPU_AoasvcProc_VirtualAntennaElements *virtAntElemList)
{
    int32_t  retVal = 0;
    uint32_t baseAddr, regionId;
    uint32_t radarCubeAddr;
    int32_t  dopplerIndUnsigned;

    baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    DebugP_assert(baseAddr != 0);
    regionId = EDMA_getRegionId(obj->edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);


    /* Set EDMA ping or pong of Input Source address */
    if (idx < numPointsMajor)
    {
        radarCubeAddr = (uint32_t)obj->radarCube[0].data;
    }
    else
    {
        radarCubeAddr = (uint32_t)obj->radarCube[1].data;
    }
    retVal = DPEDMA_setSourceAddress(obj->edmaHandle,
                                     obj->edmaIn[idx & 0x1].channel,
                                     radarCubeAddr + detObjIndOut[idx].rangeInd * sizeof(cmplx16ImRe_t));
    if (retVal != 0)
    {
        goto exit;
    }

    /* Set EDMA ping or pong of Output Source address */
    if (detObjIndOut[idx].dopplerInd < 0)
    {
        dopplerIndUnsigned = (int32_t)detObjIndOut[idx].dopplerInd + (int32_t)obj->numDopplerBins;
    }
    else
    {
        dopplerIndUnsigned = (int32_t)detObjIndOut[idx].dopplerInd;
    }
    retVal = DPEDMA_setSourceAddress(obj->edmaHandle,
                                     obj->edmaOut[idx & 0x1].channel,
                                     (uint32_t)obj->hwaMemBankAddr[2 + (idx & 0x1)] + dopplerIndUnsigned * sizeof(cmplx32ImRe_t) * obj->numVirtualAntennas);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Fill range index */
    virtAntElemList[idx].rangeInd = detObjIndOut[idx].rangeInd;
    virtAntElemList[idx].dopInd   = detObjIndOut[idx].dopplerInd;

    /* Trigger EDMA transfer from radar cube to HWA */
    EDMAEnableTransferRegion(baseAddr, regionId, obj->edmaIn[idx & 0x1].channel, EDMA_TRIG_MODE_MANUAL);

    if (retVal != 0)
    {
        goto exit;
    }

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n Configures Output EDMA
 *
 *  @param[in]   obj             DPU internal object
 *  @param[in]   detMatrix       Structure descriptor of the output detection matrix
 *
 *  \ingroup    DPU_AOASVCPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t aoasvcProc_setOutputEdmaCcnt(DPU_AoasvcProc_Obj *obj,
                                     uint32_t            cCount)
{
    int32_t retVal = 0;

    retVal = DPEDMA_setCcnt(obj->edmaHandle,
                            (uint8_t)obj->edmaOut[0].channel,
                            cCount);
    retVal = DPEDMA_setCcnt(obj->edmaHandle,
                            (uint8_t)obj->edmaOut[1].channel,
                            cCount);
    return retVal;
}

void aoasvcProc_waitEdma(DPU_AoasvcProc_Obj *obj,
                         uint8_t             channel)
{
    uint32_t baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    uint32_t regionId = EDMA_getRegionId(obj->edmaHandle);


    /* wait until transfer done */
    volatile uint32_t isTransferDone;
    do
    {
        isTransferDone = EDMAReadIntrStatusRegion(baseAddr, regionId, channel);
        if (isTransferDone)
        {
            EDMAClrIntrRegion(baseAddr, regionId, channel);
            break;
        }
    } while (isTransferDone == 0);
}

volatile int gDbgEnableSteerVecCorr = 1; // ToDo Remove this, it is for debugging

int32_t magnitude(int32_t re, int32_t im)
{
    int32_t u, v;
    int32_t p, q;
    int32_t mag;
    re = abs(re);
    im = abs(im);
    if (re > im)
    {
        u = re;
        v = im;
    }
    else
    {
        u = im;
        v = re;
    }
    p = u + (v >> 3);
    q = (7 * u + 4 * v) >> 3;
    if (p > q)
    {
        mag = p;
    }
    else
    {
        mag = q;
    }
    return mag;
}

#if AOASVC_PROFILE_ENABLE
uint32_t gStVecCorPrfile[5];
#endif

/**
 *  @b Description
 *  @n Correction of the coordinates of detected points in the point cloud list. Applies
 *     steering vectors to the regenerated antenna symbols of the detected point, finds the maximum
 *     in the computed azimuth-elevation heatmap, performs position interpolation, and computes x,y,z of the
 *     point and overwrites the existing coordinates.
 *
 *
 *  @param[in]       obj            DPU object
 *  @param[in]       numPoints      Number of detected points
 *  @param[in/out]   detObjOut      Point-cloud list with detected objets
 *  @param[in]      detObjIndOut    Range/doppler indices of the detected points in the point-cloud list
 *  @param[in]      outParams       List with antenna symbols of the detected points in the point-cloud list
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t aoasvcProc_SteeringVecCorrection(DPU_AoasvcProc_Obj               *obj,
                                         uint32_t                          numPoints,
                                         DPIF_PointCloudCartesianExt      *detObjOut,
                                         DPIF_PointCloudRngAzimElevDopInd *detObjIndOut,
                                         DPU_AoasvcProc_OutParams         *outParams)
{
    int32_t                      retVal = 0;
    HWA_CommonConfig             hwaCommonConfig;
    int32_t                      idx;
    DPU_AoasvcProc_HwaMaxOutput *peakPointer = (DPU_AoasvcProc_HwaMaxOutput *)(CSL_APP_HWA_DMA0_RAM_BANK2_BASE + CSL_APP_HWA_BANK_SIZE - sizeof(DPU_AoasvcProc_HwaMaxOutput));
    int32_t                      peakInd, azimPeakInd, elevPeakInd;
    float                        azimuth;
    float                        elevation;
    float                        range;
    cmplx32ImRe_t               *detMat = (cmplx32ImRe_t *)CSL_APP_HWA_DMA0_RAM_BANK3_BASE;

    uint32_t baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    uint32_t regionId = EDMA_getRegionId(obj->edmaHandle);

    if (numPoints == 0)
    {
        goto exit;
    }

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                     aoasvcProc_hwaDoneIsrCallback,
                                     (void *)&obj->hwaDoneSemaHandle);
    if (retVal != 0)
    {
        goto exit;
    }
    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    memset((void *)&hwaCommonConfig, 0, sizeof(HWA_CommonConfig));

    /* Config Common Registers */
    hwaCommonConfig.configMask =
        HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_I_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_Q_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_FFTSUMDIV;

    hwaCommonConfig.numLoops                        = 1;
    hwaCommonConfig.paramStartIdx                   = obj->steerVecHwaParamCfg.hwaParamStartIdx;
    hwaCommonConfig.paramStopIdx                    = obj->steerVecHwaParamCfg.hwaParamStopIdx;
    hwaCommonConfig.fftConfig.fft1DEnable           = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    hwaCommonConfig.fftConfig.fftSumDiv             = obj->dopFftSumDiv;

    retVal = HWA_configCommon(obj->hwaHandle, &hwaCommonConfig);
    if (retVal != 0)
    {
        goto exit;
    }

    uint32_t startTime = Cycleprofiler_getTimeStamp();

    /* Trigger EDMA transfer from radar cube to HWA */
    EDMAEnableTransferRegion(baseAddr, regionId, obj->edmaInSteerVec.channel, EDMA_TRIG_MODE_MANUAL);
    aoasvcProc_waitEdma(obj, obj->edmaInSteerVec.channel);

#if AOASVC_PROFILE_ENABLE
    gStVecCorPrfile[0] = Cycleprofiler_getTimeStamp() - startTime;
#endif

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                     aoasvcProc_hwaDoneIsrCallback,
                                     (void *)&obj->hwaDoneSemaHandle);
    if (retVal != 0)
    {
        goto exit;
    }

    /**********************************************/
    /* Correlate points with steering vectors     */
    /**********************************************/
    for (idx = 0; idx < numPoints; idx++)
    {
        if (idx == 0)
        {
            startTime = Cycleprofiler_getTimeStamp();
        }
        /* Load point antenna symbols to internal RAM */
        retVal = HWA_configRam(obj->hwaHandle,
                               HWA_RAM_TYPE_INTERNAL_RAM,
                               (uint8_t *)outParams->virtAntElemList[idx].antenna,
                               obj->numVirtualAntennas * sizeof(cmplx32ImRe_t), // size in bytes
                               0);
        if (retVal != 0)
        {
            goto exit;
        }

#if AOASVC_PROFILE_ENABLE
        if (idx == 0)
        {
            gStVecCorPrfile[1] = Cycleprofiler_getTimeStamp() - startTime;
            startTime          = Cycleprofiler_getTimeStamp();
        }
#endif
        /* Enable HWA */
        retVal = HWA_enable(obj->hwaHandle, 1);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Trigger HWA */
        retVal = HWA_setSoftwareTrigger(obj->hwaHandle);
        if (retVal != 0)
        {
            goto exit;
        }

        /**********************************************/
        /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
        /**********************************************/
        retVal = SemaphoreP_pend(&obj->hwaDoneSemaHandle, SystemP_WAIT_FOREVER);
        if (retVal != SystemP_SUCCESS)
        {
            retVal = DPU_AOASVCPROC_ESEMASTATUS;
            goto exit;
        }

        /* Disable the HWA */
        retVal = HWA_enable(obj->hwaHandle, 0);
        if (retVal != 0)
        {
            goto exit;
        }

        /* read maximum position from the azimuht-elevation heatmap */
        peakInd     = peakPointer->maxInd;
        azimPeakInd = (peakInd % obj->numAzimVec);
        elevPeakInd = (peakInd / obj->numAzimVec);
        azimuth     = SteeringVecParams.azimSign * (azimPeakInd * obj->azimStep + obj->azimStart);
        elevation   = SteeringVecParams.elevSign * (elevPeakInd * obj->elevStep + obj->elevStart);

#if AOASVC_PROFILE_ENABLE
        if (idx == 0)
        {
            gStVecCorPrfile[2] = Cycleprofiler_getTimeStamp() - startTime;
            startTime          = Cycleprofiler_getTimeStamp();
        }
#endif
        /* Azimuth elevation interpolation */
        if (obj->enableAngleInterpolation)
        {
            int32_t azimLeftInd, azimRightInd, elevLeftInd, elevRightInd;
            int32_t peakVal, azimLeftVal, azimRightVal, elevLeftVal, elevRightVal;
            float   azimOffset, elevOffset;

            peakVal = peakPointer->peak;

            if ((azimPeakInd == 0) || (azimPeakInd == (obj->numAzimVec - 1)))
            {
                azimOffset = 0;
            }
            else
            {
                azimLeftInd  = peakInd - 1;
                azimRightInd = peakInd + 1;
                azimLeftVal  = magnitude(detMat[azimLeftInd].real, detMat[azimLeftInd].imag);
                azimRightVal = magnitude(detMat[azimRightInd].real, detMat[azimRightInd].imag);
                azimOffset   = (float)(azimRightVal - azimLeftVal) / (float)(4 * peakVal - 2 * azimRightVal - 2 * azimLeftVal);
            }
            azimuth += SteeringVecParams.azimSign * azimOffset * obj->azimStep;

            if ((elevPeakInd == 0) || (elevPeakInd == (obj->numElevVec - 1)))
            {
                elevOffset = 0;
            }
            else
            {
                elevLeftInd  = peakInd - obj->numAzimVec;
                elevRightInd = peakInd + obj->numAzimVec;
                elevLeftVal  = magnitude(detMat[elevLeftInd].real, detMat[elevLeftInd].imag);
                elevRightVal = magnitude(detMat[elevRightInd].real, detMat[elevRightInd].imag);
                elevOffset   = (float)(elevRightVal - elevLeftVal) / (float)(4 * peakVal - 2 * elevRightVal - 2 * elevLeftVal);
            }
            elevation += SteeringVecParams.elevSign * elevOffset * obj->elevStep;
        }
#if AOASVC_PROFILE_ENABLE
        if (idx == 0)
        {
            gStVecCorPrfile[3] = Cycleprofiler_getTimeStamp() - startTime;
            startTime          = Cycleprofiler_getTimeStamp();
        }
#endif
        /* Correct point coordinates in the point-cloud list */
        range            = obj->rangeStep * outParams->virtAntElemList[idx].rangeInd;
        detObjOut[idx].x = range * cosf(elevation) * sinf(azimuth);
        detObjOut[idx].y = range * cosf(elevation) * cosf(azimuth);
        detObjOut[idx].z = range * sinf(elevation);

#if AOASVC_PROFILE_ENABLE
        if (idx == 0)
        {
            gStVecCorPrfile[4] = Cycleprofiler_getTimeStamp() - startTime;
            startTime          = Cycleprofiler_getTimeStamp();
        }
#endif
    }

    /* Disable interrupt */
    HWA_disableDoneInterrupt(obj->hwaHandle);

exit:
    return retVal;
}

#if AOASVC_PROFILE_ENABLE
uint32_t gAoaosvcCpuCycles[101];
uint32_t gAntennaSymbolsGenLoopTime[101];
#endif

/**
 *  @b Description
 *  @n AOASVC DPU process function. Computes the range/azimuth detection matrix
 *
 *  @param[in]   handle        DPU handle
 *  @param[in]   radarCubeSrc  Structure descriptor of the input radar cube
 *  @param[in]   detMatrix     Pointer to output detection matrix.
 *  @param[out]  outParams     Output parameters.
 *
 *  \ingroup    DPU_AOASVCPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t DPU_AoasvcProc_process(
    DPU_AoasvcProc_Handle             handle,
    uint32_t                          numPointsMajor,
    uint32_t                          numPointsMinor,
    DPIF_PointCloudCartesianExt      *detObjOut,
    DPIF_PointCloudRngAzimElevDopInd *detObjIndOut,
    DPU_AoasvcProc_OutParams         *outParams)
{
    uint32_t                               startTime = Cycleprofiler_getTimeStamp();
    DPU_AoasvcProc_Obj                    *obj;
    int32_t                                retVal = 0;
    uint32_t                               numPoints, numPointsEven, numLoops;
    uint32_t                               inIdx, outIdx;
    DPU_AoasvcProc_VirtualAntennaElements *virtAntElemList = outParams->virtAntElemList;

    bool             status;
    HWA_CommonConfig hwaCommonConfig;

    numPointsEven = 0;
    obj           = (DPU_AoasvcProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }
    /* Set inProgress state */
    obj->inProgress = true;

    numPoints = numPointsMajor + numPointsMinor;
    if (numPoints == 0)
    {
        goto exit;
    }
    numLoops      = (numPoints + 1) >> 1;
    numPointsEven = 2 * numLoops;
    if (numPointsEven > numPoints)
    {
        // Force range,doppler of the last dummy to zero, when number of points is odd
        detObjIndOut[numPointsEven - 1].rangeInd   = 0;
        detObjIndOut[numPointsEven - 1].dopplerInd = 0;
    }

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                     aoasvcProc_hwaDoneIsrCallback,
                                     (void *)&obj->hwaDoneSemaHandle);
    if (retVal != 0)
    {
        goto exit;
    }
    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    memset((void *)&hwaCommonConfig, 0, sizeof(HWA_CommonConfig));

    /* Config Common Registers */
    hwaCommonConfig.configMask =
        HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_I_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_Q_CMULT_SCALE |
        HWA_COMMONCONFIG_MASK_FFTSUMDIV;

    hwaCommonConfig.numLoops                        = numLoops;
    hwaCommonConfig.paramStartIdx                   = obj->dopplerHwaParamCfg.hwaParamStartIdx;
    hwaCommonConfig.paramStopIdx                    = obj->dopplerHwaParamCfg.hwaParamStopIdx;
    hwaCommonConfig.fftConfig.fft1DEnable           = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    hwaCommonConfig.fftConfig.fftSumDiv             = obj->dopFftSumDiv;

    retVal = HWA_configCommon(obj->hwaHandle, &hwaCommonConfig);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Enable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 1);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Configure Output EDMA ping/pong to run each numLoops times */
    aoasvcProc_setOutputEdmaCcnt(obj, numLoops);


    /* Configure Input/Output for point 0 */
    inIdx = 0;
    aoasvcProc_setInputOutputEdma(obj, inIdx, detObjIndOut, numPointsMajor, virtAntElemList);
    inIdx++;

    /* Configure Input/Output for point 1 */
    aoasvcProc_setInputOutputEdma(obj, inIdx, detObjIndOut, numPointsMajor, virtAntElemList);
    inIdx++;

    for (outIdx = 0; outIdx < numPointsEven; outIdx++)
    {
#if 0 // EDMA_INT
        /**********************************************/
        /* WAIT FOR EDMA DONE INTERRUPT               */
        /**********************************************/
        status = SemaphoreP_pend(&obj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);
        if (status != SystemP_SUCCESS)
        {
            retVal = DPU_AOASVCPROC_ESEMASTATUS;
            goto exit;
        }
#else
        aoasvcProc_waitEdma(obj, obj->edmaOut[outIdx & 0x1].channel);
#endif

        if (inIdx < numPointsEven)
        {
            aoasvcProc_setInputOutputEdma(obj, inIdx, detObjIndOut, numPointsMajor, virtAntElemList);
            inIdx++;
        }
    }

    /**********************************************/
    /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
    /**********************************************/
    status = SemaphoreP_pend(&obj->hwaDoneSemaHandle, SystemP_WAIT_FOREVER);
    if (status != SystemP_SUCCESS)
    {
        retVal = DPU_AOASVCPROC_ESEMASTATUS;
        goto exit;
    }

    HWA_disableDoneInterrupt(obj->hwaHandle);

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }
    gAntennaSymbolsGenLoopTime[numPointsEven] = Cycleprofiler_getTimeStamp() - startTime;

    /* Correct position of detected points using steering vector approach*/
    if (obj->enableSteeringVectorCorrection & gDbgEnableSteerVecCorr)
    {
        retVal = aoasvcProc_SteeringVecCorrection(obj,
                                                  numPoints,
                                                  detObjOut,
                                                  detObjIndOut,
                                                  outParams);
        if (retVal != 0)
        {
            goto exit;
        }
        // gDbgEnableSteerVecCorr = 0;
    }

exit:
    outParams->stats.numProcess++;
    outParams->stats.processingTime  = Cycleprofiler_getTimeStamp() - startTime;
    gAoaosvcCpuCycles[numPointsEven] = outParams->stats.processingTime;
    if (obj != NULL)
    {
        obj->inProgress = false;
    }

    return retVal;
}

/**
 *  @b Description
 *  @n
 *  Doppler DPU deinit
 *
 *  @param[in]   handle   DPU handle.
 *
 *  \ingroup    DPU_AOASVCPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success      =0
 *  @retval
 *      Error       !=0 @ref DPU_DOPPLERPROC_ERROR_CODE
 */
int32_t DPU_AoasvcProc_deinit(DPU_AoasvcProc_Handle handle)
{
    int32_t retVal = 0;

    /* Free memory */
    if (handle == NULL)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
    }

    return retVal;
}

/**
 *  @b Description
 *  @n
 *   Returns number of allocated HWA Param sets
 *
 *  @param[in]   handle     DPU handle.
 *  @param[out]   cfg       Number of allocated HWA Param sets
 *
 *  @retval
 *      Success      = 0
 *  @retval
 *      Error       != 0
 */
int32_t DPU_AoasvcProc_GetNumUsedHwaParamSets(
    DPU_AoasvcProc_Handle handle,
    uint8_t              *numUsedHwaParamSets)
{
    DPU_AoasvcProc_Obj *obj;
    int32_t             retVal = 0;

    obj = (DPU_AoasvcProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_AOASVCPROC_EINVAL;
        goto exit;
    }
    *numUsedHwaParamSets = (uint8_t)(obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1);
exit:
    return retVal;
}
