/**
 *   @file  cfarprochwa.c
 *
 *   @brief
 *      Implements Data path processing Unit using HWA.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018-2023 Texas Instruments, Inc.
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

/* mmWave SDK drivers/common Include Files */
#include <common/syscommon.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>
#include <kernel/dpl/HeapP.h>
#include <drivers/edma.h>
//#include <ti/drivers/esm/esm.h>
#include <drivers/soc.h>
#include <drivers/hw_include/hw_types.h>

/* Data Path Include Files */
#include "dpedma/v1/dpedmahwa.h"
#include "cfarprochwa.h"
#include "cfarprochwainternal.h"


#define DBG_CFAR_HWA_OBJ_DPU

/* @brief definition for detection matrix data QFormat */
#define CFAR_HWA_DETMATRIX_DATA_QFORMAT 11

#ifdef DBG_CFAR_HWA_OBJ_DPU
volatile CFARHwaObj *gCfarHwaObj = NULL;
#endif

/* User defined heap memory and handle */
#define CFARPROC_HEAP_MEM_SIZE (sizeof(CFARHwaObj))

static uint8_t gCfarProcHeapMem[CFARPROC_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function as per HWA API.
 *      Depending on the programmed transfer completion codes,
 *      posts HWA done semaphore.
 *
 *  @param[in]  arg                 Argument to the callback function
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *  @retval     N/A
 */
static void CFARCAHWADoneIsrCallback(void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/** @brief Configures ParameterSet for CFAR detection (The CFAR detection runs along range bins).
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *   @param[in] pRes                Pointer to hardware resources
 *
 *   @param[in] handle              HWA driver handle
 *
 *   @param[in] paramSetStartIdx    HWA parameter set start index
 *
 *   @param[in] numRangeBins        Number of range bins
 *
 *   @param[in] numCfarRunIterations Number of iterations (number of azimuth bins or number of doppler bins)
 *
 *   @param[in] cfarCfg             Pointer to CFAR HWA configuration
 *
 *   @param[in] detObjectListSize   Maximum size of the list of detected objects
 *
 *   @param[in] dmaTriggerSource    DMA trigger source channel
 *
 *   @param[in] hwaSourceBufOffset  HWA memory offset with input data
 *
 *   @param[in] hwaDestBufOffset    HWA memory offset with CFAR output results:
 *                                  List of detected objects as array of structures
 *                                  cfarDetOutput_t
 *
 *   @param[in] hwaDestBufOffsetRangeProfile HWA memory offset for Range Profile
 *
 *   @param[in] cfarDomain          0: CFAR in range domain, 1: CFAR in Doppler domain
 *                                  cfarDetOutput_t
 *   @param[in] isDetMatrixLogScale 1: input matrix is log magnitude, 0: input matrix is magnitude
 */
int32_t CFAR_DET_HWA_config_HWA(DPU_CFARProcHWA_HW_Resources *pRes,
                                CFARHwaObj                   *cfarHwaObj,
                                uint8_t                       paramSetStartIdx,
                                uint32_t                      numRangeBins,
                                uint32_t                      numCfarRunIterations,
                                DPU_CFARProc_CfarCfg         *cfarCfg,
                                DPU_CFARProc_CfarScndPassCfg *cfarScndPassCfg,
                                uint16_t                      detObjectListSize,
                                uint8_t                       dmaTriggerSource,
                                uint16_t                      hwaSourceBufOffset,
                                uint16_t                      hwaDestBufOffset,
                                uint16_t                      hwaDestBufOffsetRangeProfile,
                                uint8_t                       cfarDomain,
                                bool                          isDetMatrixLogScale)
{
    HWA_ParamConfig     hwaParamCfg;
    int32_t             retVal = 0;
    uint32_t            cfarMaxNumDetPoints;
    HWA_InterruptConfig paramISRConfig;
    uint8_t             paramSetCurrentIdx;
    int16_t             detMatrixElemLenInBytes;
    uint8_t             destChan;
    HWA_Handle          handle = cfarHwaObj->hwaHandle;

    if (isDetMatrixLogScale)
    {
        detMatrixElemLenInBytes = sizeof(uint16_t);
    }
    else
    {
        detMatrixElemLenInBytes = sizeof(uint32_t);
    }

    if (numRangeBins * numCfarRunIterations * detMatrixElemLenInBytes > 2 * (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS))
    {
        retVal = DPU_CFARPROCHWA_ENOMEM__DET_MATRIX_EXCEEDS_HWA_INP_MEM;
        goto exit;
    }

    cfarMaxNumDetPoints = detObjectListSize;

    retVal = HWA_enable(handle, 0); // Disable HWA
    if (retVal != 0)
    {
        goto exit;
    }

    paramSetCurrentIdx = paramSetStartIdx;
    /***********************************/
    /* Configure Maximum per range bin */
    /***********************************/
    memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
    hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
    hwaParamCfg.dmaTriggerSrc = dmaTriggerSource;

    hwaParamCfg.accelMode = HWA_ACCELMODE_FFT;

    hwaParamCfg.source.srcAcnt = numCfarRunIterations - 1;
    hwaParamCfg.source.srcAIdx = detMatrixElemLenInBytes;
    hwaParamCfg.source.srcBIdx = numCfarRunIterations * detMatrixElemLenInBytes;
    if (isDetMatrixLogScale)
    {
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcScale = 8;
    }
    else
    {
        hwaParamCfg.source.srcWidth = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcScale = 0;
    }
    hwaParamCfg.source.srcBcnt        = numRangeBins - 1;
    hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
    hwaParamCfg.source.srcAddr        = hwaSourceBufOffset;

    hwaParamCfg.dest.dstAddr        = hwaDestBufOffsetRangeProfile;
    hwaParamCfg.dest.dstAcnt        = 4095;
    hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
    hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.dest.dstAIdx        = sizeof(DPU_CFARProcHWA_CfarDetOutput);
    hwaParamCfg.dest.dstBIdx        = sizeof(DPU_CFARProcHWA_CfarDetOutput);
    hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
    hwaParamCfg.dest.dstScale       = 8;

    hwaParamCfg.accelModeArgs.fftMode.fftEn      = 0;
    hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_MAX_STATS;

    retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Disable trigger DMA/interrupt for this param set */
    retVal = HWA_disableParamSetInterrupt(handle,
                                          paramSetCurrentIdx,
                                          HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
    if (retVal != 0)
    {
        goto exit;
    }
    paramSetCurrentIdx++;

    /***************************/
    /* Configure CFAR Paramset */
    /***************************/
    memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
    hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
    hwaParamCfg.dmaTriggerSrc = 0;

    hwaParamCfg.accelModeArgs.cfarMode.peakGroupEn = cfarCfg->peakGroupingEn;
    hwaParamCfg.accelMode                          = HWA_ACCELMODE_CFAR;

    hwaParamCfg.source.srcAcnt = numRangeBins - 1;
    hwaParamCfg.source.srcAIdx = numCfarRunIterations * detMatrixElemLenInBytes;
    hwaParamCfg.source.srcBIdx = detMatrixElemLenInBytes;
    if (isDetMatrixLogScale)
    {
        hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_LOG_INPUT_REAL;
        hwaParamCfg.source.srcWidth                 = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcScale                 = 8;
    }
    else
    {
        hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_MAG_INPUT_REAL;
        hwaParamCfg.source.srcWidth                 = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcScale                 = 0;
    }
    hwaParamCfg.source.srcBcnt        = numCfarRunIterations - 1;
    hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
    hwaParamCfg.source.srcAddr        = hwaSourceBufOffset;

    hwaParamCfg.dest.dstAddr        = hwaDestBufOffset;
    hwaParamCfg.dest.dstAcnt        = cfarMaxNumDetPoints - 1;
    hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
    hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
    hwaParamCfg.dest.dstAIdx        = sizeof(DPU_CFARProcHWA_CfarDetOutput);
    hwaParamCfg.dest.dstBIdx        = 0; // don't care
    hwaParamCfg.dest.dstScale       = 8;

    hwaParamCfg.accelModeArgs.cfarMode.numGuardCells        = cfarCfg->guardLen;
    hwaParamCfg.accelModeArgs.cfarMode.nAvgDivFactor        = cfarCfg->noiseDivShift;
    hwaParamCfg.accelModeArgs.cfarMode.cyclicModeEn         = cfarCfg->cyclicMode;
    hwaParamCfg.accelModeArgs.cfarMode.nAvgMode             = cfarCfg->averageMode;
    hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesRight = cfarCfg->winLen >> 1;
    hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesLeft  = cfarCfg->winLen >> 1;
    hwaParamCfg.accelModeArgs.cfarMode.outputMode           = HWA_CFAR_OUTPUT_MODE_I_PEAK_IDX_Q_NEIGHBOR_NOISE_VAL;

    retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Disable trigger DMA/interrupt for this param set */
    retVal = HWA_disableParamSetInterrupt(handle,
                                          paramSetCurrentIdx,
                                          HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
    if (retVal != 0)
    {
        goto exit;
    }
    retVal = HWA_getDMAChanIndex(handle,
                                 pRes->edmaHwaOut.channel, // ToDo Add the correct output channel
                                 &destChan);

    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel    = destChan;
    paramISRConfig.cpu.callbackArg   = NULL;
    paramISRConfig.cpu.callbackFn    = NULL;

    retVal = HWA_enableParamSetInterrupt(handle,
                                         paramSetCurrentIdx,
                                         &paramISRConfig);
    if (retVal != 0)
    {
        goto exit;
    }
    paramSetCurrentIdx++;

    if (cfarScndPassCfg->enabled)
    {
        /************************************************/
        /* Configure CFAR Param set in second dimension */
        /************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_SOFTWARE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.accelModeArgs.cfarMode.peakGroupEn = cfarCfg->peakGroupingEn;
        hwaParamCfg.accelMode                          = HWA_ACCELMODE_CFAR;
        if (cfarScndPassCfg->cyclicMode)
        {
            hwaParamCfg.source.srcAcnt          = (numCfarRunIterations - 1) + (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen) + (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen);
            hwaParamCfg.source.srcShift         = numCfarRunIterations - (cfarScndPassCfg->guardLen + cfarScndPassCfg->winLen);
            hwaParamCfg.source.srcCircShiftWrap = mathUtils_floorLog2(numCfarRunIterations);
        }
        else
        {
            hwaParamCfg.source.srcAcnt          = numCfarRunIterations - 1;
            hwaParamCfg.source.srcShift         = 0;
            hwaParamCfg.source.srcCircShiftWrap = 0;
        }
        hwaParamCfg.source.srcAIdx = detMatrixElemLenInBytes;
        hwaParamCfg.source.srcBIdx = numCfarRunIterations * detMatrixElemLenInBytes;
        if (isDetMatrixLogScale)
        {
            hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_LOG_INPUT_REAL;
            hwaParamCfg.source.srcWidth                 = HWA_SAMPLES_WIDTH_16BIT;
            hwaParamCfg.source.srcScale                 = 8;
        }
        else
        {
            hwaParamCfg.accelModeArgs.cfarMode.operMode = HWA_CFAR_OPER_MODE_MAG_INPUT_REAL;
            hwaParamCfg.source.srcWidth                 = HWA_SAMPLES_WIDTH_32BIT;
            hwaParamCfg.source.srcScale                 = 0;
        }
        hwaParamCfg.source.srcBcnt        = numRangeBins - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcAddr        = hwaSourceBufOffset;

        hwaParamCfg.dest.dstAddr        = hwaDestBufOffsetRangeProfile; // write second pass CFAR list over range profile since it will be already out
        hwaParamCfg.dest.dstAcnt        = cfarMaxNumDetPoints - 1;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstAIdx        = sizeof(DPU_CFARProcHWA_CfarDetOutput);
        hwaParamCfg.dest.dstBIdx        = 0; // don't care
        hwaParamCfg.dest.dstScale       = 8;

        hwaParamCfg.accelModeArgs.cfarMode.numGuardCells        = cfarScndPassCfg->guardLen;
        hwaParamCfg.accelModeArgs.cfarMode.nAvgDivFactor        = cfarScndPassCfg->noiseDivShift;
        hwaParamCfg.accelModeArgs.cfarMode.cyclicModeEn         = cfarScndPassCfg->cyclicMode;
        hwaParamCfg.accelModeArgs.cfarMode.nAvgMode             = cfarScndPassCfg->averageMode;
        hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesRight = cfarScndPassCfg->winLen >> 1;
        hwaParamCfg.accelModeArgs.cfarMode.numNoiseSamplesLeft  = cfarScndPassCfg->winLen >> 1;
        hwaParamCfg.accelModeArgs.cfarMode.outputMode           = HWA_CFAR_OUTPUT_MODE_I_PEAK_IDX_Q_NEIGHBOR_NOISE_VAL;

        retVal = HWA_configParamSet(handle, paramSetCurrentIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Disable trigger DMA/interrupt for this param set */
        retVal = HWA_disableParamSetInterrupt(handle,
                                              paramSetCurrentIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramSetCurrentIdx++;
    }


    /* Save number of PARAM sets used */
    pRes->hwaCfg.numParamSet = paramSetCurrentIdx - pRes->hwaCfg.paramSetStartIdx;
#if 0

    /* Save HWA Params ToDo make this as in DOA and UDOP DPUs */
    if (pRes->hwaCfg.hwaParamsSaveLoc.sizeBytes < (pRes->hwaCfg.numParamSet * HWA_NUM_REG_PER_PARAM_SET * sizeof(uint32_t)))
    {
        retVal = DPU_CFARPROCHWA_EHWA_PARAM_SAVE_LOC_SIZE;
        goto exit;
    }
    retVal = HWA_saveParams(handle,
                            pRes->hwaCfg.paramSetStartIdx,
                            pRes->hwaCfg.numParamSet,
                            pRes->hwaCfg.hwaParamsSaveLoc.data);
#endif

exit:
    return (retVal);
}

/**
 *  @b Description
 *  @n
 *      EDMA completion call back function.
 *  \ingroup    DPU_DOPPLERPROC_INTERNAL_FUNCTION
 */
static void CFAR_DET_HWA_edmaDoneIsrCallback(Edma_IntrHandle intrHandle, void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}

/**
 *  @b Description
 *  @n
 *      Configures all CFAR processing related EDMA configuration.
 *
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *  @param[in] hwaHandle HWA handle
 *
 *  @param[in] pRes Pointer to hardware resources
 *
 *  @param[in] staticCfg Pointer to static configuration
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t CFAR_DET_HWA_config_EDMA(
    CFARHwaObj                   *obj,
    HWA_Handle                    hwaHandle,
    DPU_CFARProcHWA_HW_Resources *pRes,
    DPU_CFARProcHWA_StaticConfig *staticCfg)
{
    int32_t            errorCode  = SystemP_SUCCESS;
    EDMA_Handle        edmaHandle = pRes->edmaHandle;
    DPEDMA_ChainingCfg chainingCfg;
    DPEDMA_syncACfg    syncACfg;
    int16_t            detMatrixElemLenInBytes;
    uint32_t           numAzimuthBins;

    bool               isTransferCompletionEnabled;
    Edma_EventCallback transferCompletionCallbackFxn    = NULL;
    void              *transferCompletionCallbackFxnArg = NULL;
    DPEDMA_syncABCfg   syncABCfg;
    uint32_t           numCfarRunIterations;


    /******************************************************************************************/
    /**************                     PROGRAM EDMA INPUT                    *****************/
    /******************************************************************************************/
    if (staticCfg->isDetMatrixLogScale)
    {
        detMatrixElemLenInBytes = sizeof(uint16_t);
    }
    else
    {
        detMatrixElemLenInBytes = sizeof(uint32_t);
    }
    if (staticCfg->angleDimension > 0)
    {
        /* 1D or 2D */
        numAzimuthBins = staticCfg->azimuthFftSize;
    }
    else
    {
        /* 0D  (1Tx-1Rx) */
        numAzimuthBins = 1;
    }

    /* Check the type of input heat-map */
    if (staticCfg->detectionHeatmapType == DPU_CFAR_RANGE_DOPPLER_HEATMAP)
    {
        numCfarRunIterations = staticCfg->numDopplerBins;
    }
    else if (staticCfg->detectionHeatmapType == DPU_CFAR_RANGE_AZIMUTH_HEATMAP)
    {
        numCfarRunIterations = numAzimuthBins;
    }
    else
    {
        errorCode = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    if ((staticCfg->numRangeBins * numCfarRunIterations * detMatrixElemLenInBytes) > (2 * (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS)))
    {
        errorCode = DPU_CFARPROCHWA_ENOMEM__DET_MATRIX_EXCEEDS_HWA_INP_MEM;
        goto exit;
    }

    chainingCfg.chainingChan                  = pRes->edmaHwaInSignature.channel;
    chainingCfg.isIntermediateChainingEnabled = false;
    chainingCfg.isFinalChainingEnabled        = true;

    syncACfg.srcAddress  = (uint32_t)pRes->detMatrix.data;
    syncACfg.destAddress = (uint32_t)pRes->hwaMemInp;
    syncACfg.aCount      = staticCfg->numRangeBins * numCfarRunIterations * detMatrixElemLenInBytes;
    syncACfg.bCount      = 1;
    syncACfg.srcBIdx     = 0;
    syncACfg.dstBIdx     = 0;

    errorCode = DPEDMA_configSyncA_singleFrame(edmaHandle,
                                               &pRes->edmaHwaIn,
                                               &chainingCfg,
                                               &syncACfg,
                                               false, // isEventTriggered
                                               false, // isIntermediateTransferInterruptEnabled
                                               false, // isTransferCompletionEnabled
                                               NULL, // transferCompletionCallbackFxn
                                               NULL, // transferCompletionCallbackFxnArg
                                               NULL);

    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

    errorCode = DPEDMAHWA_configOneHotSignature(edmaHandle,
                                                &pRes->edmaHwaInSignature,
                                                hwaHandle,
                                                pRes->hwaCfg.dmaTrigSrcChan,
                                                false);
    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************/
    /**************                     PROGRAM EDMA OUTPUT                   *****************/
    /******************************************************************************************/
    chainingCfg.chainingChan                  = pRes->edmaHwaOut.channel;
    chainingCfg.isIntermediateChainingEnabled = false;
    chainingCfg.isFinalChainingEnabled        = false;

    isTransferCompletionEnabled      = true;
    transferCompletionCallbackFxn    = CFAR_DET_HWA_edmaDoneIsrCallback;
    transferCompletionCallbackFxnArg = (void *)&(obj->edmaDoneSemaHandle);

    syncABCfg.srcAddress  = ((uint32_t)pRes->hwaMemOutRangeProfile) + sizeof(uint32_t); // Skip index field, position to first peak field
    syncABCfg.destAddress = (uint32_t)0; // Temporary set to zero, will be set on the fly
    syncABCfg.aCount      = sizeof(uint32_t);
    syncABCfg.bCount      = staticCfg->numRangeBins;
    syncABCfg.cCount      = 1;
    syncABCfg.srcBIdx     = sizeof(DPU_CFARProcHWA_HwaMaxOutput);
    syncABCfg.srcCIdx     = 0;
    syncABCfg.dstBIdx     = sizeof(uint32_t);
    syncABCfg.dstCIdx     = 0;

    errorCode = DPEDMA_configSyncAB(edmaHandle,
                                    &pRes->edmaHwaOut,
                                    &chainingCfg,
                                    &syncABCfg,
                                    true, // isEventTriggered
                                    false, // isIntermediateTransferInterruptEnabled
                                    isTransferCompletionEnabled,
                                    transferCompletionCallbackFxn,
                                    transferCompletionCallbackFxnArg,
                                    pRes->intrObj);
    if (errorCode != SystemP_SUCCESS)
    {
        goto exit;
    }


exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *      Prepares HWA during run time. Configures common registers. If the HWA
 *      param sets are not reused, they can be programmed one time, and only
 *      this function has to be called during run time.
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 *  @param[in] cfarHwaObj Pointer to CFAR DPU instance
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t CFAR_DET_HWA_prepareHwaRunTime(
    CFARHwaObj *cfarHwaObj)
{
    int32_t               retVal = 0;
    HWA_CommonConfig      hwaCommonConfig;
    DPU_CFARProc_CfarCfg *pCfarCfg;
    uint8_t               paramSetStartIdx;
    uint8_t               paramSetEndIdx;

    DPU_CFARProcHWA_HW_Resources *pRes = &cfarHwaObj->res;

    pCfarCfg         = &cfarHwaObj->cfarCfg;
    paramSetStartIdx = pRes->hwaCfg.paramSetStartIdx;
    paramSetEndIdx   = pRes->hwaCfg.paramSetStartIdx + 1;


    /* Disable the HWA */
    retVal = HWA_enable(cfarHwaObj->hwaHandle, 0); // set 1 to enable
    if (retVal != 0)
    {
        goto exit;
    }

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_CFARTHRESHOLDSCALE;

    hwaCommonConfig.numLoops                        = 1;
    hwaCommonConfig.paramStartIdx                   = paramSetStartIdx;
    hwaCommonConfig.paramStopIdx                    = paramSetEndIdx;
    hwaCommonConfig.fftConfig.fft1DEnable           = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    hwaCommonConfig.cfarConfig.cfarThresholdScale   = pCfarCfg->thresholdScale;

    retVal = HWA_configCommon(cfarHwaObj->hwaHandle, &hwaCommonConfig);

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *      Prepares HWA during run time. Configures common registers. If the HWA
 *      param sets are not reused, they can be programmed one time, and only
 *      this function has to be called during run time.
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 *  @param[in] cfarHwaObj Pointer to CFAR DPU instance
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t CFAR_DET_HWA_prepareHwaRunTimeScndPass(
    CFARHwaObj *cfarHwaObj)
{
    int32_t                       retVal = 0;
    HWA_CommonConfig              hwaCommonConfig;
    DPU_CFARProc_CfarScndPassCfg *pCfarCfg;
    uint8_t                       paramSetStartIdx;
    uint8_t                       paramSetEndIdx;

    DPU_CFARProcHWA_HW_Resources *pRes = &cfarHwaObj->res;

    pCfarCfg         = &cfarHwaObj->cfarScndPassCfg;
    paramSetStartIdx = pRes->hwaCfg.paramSetStartIdx + 2;
    paramSetEndIdx   = pRes->hwaCfg.paramSetStartIdx + 2;


    /* Disable the HWA */
    retVal = HWA_enable(cfarHwaObj->hwaHandle, 0); // set 1 to enable
    if (retVal != 0)
    {
        goto exit;
    }

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS |
        HWA_COMMONCONFIG_MASK_PARAMSTARTIDX |
        HWA_COMMONCONFIG_MASK_PARAMSTOPIDX |
        HWA_COMMONCONFIG_MASK_FFT1DENABLE |
        HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD |
        HWA_COMMONCONFIG_MASK_CFARTHRESHOLDSCALE;

    hwaCommonConfig.numLoops                        = 1;
    hwaCommonConfig.paramStartIdx                   = paramSetStartIdx;
    hwaCommonConfig.paramStopIdx                    = paramSetEndIdx;
    hwaCommonConfig.fftConfig.fft1DEnable           = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    hwaCommonConfig.cfarConfig.cfarThresholdScale   = pCfarCfg->thresholdScale;

    retVal = HWA_configCommon(cfarHwaObj->hwaHandle, &hwaCommonConfig);

exit:
    return retVal;
}

#define DPU_CFAR_EDMA_TPCC_SRC(n) ((uint32_t)0x4004U + ((n)*32U))
#define DPU_CFAR_EDMA_TPCC_DST(n) ((uint32_t)0x400CU + ((n)*32U))

/**
 *  @b Description
 *  @n
 *      Prepares EDMA at run time: sets source address of input EDMA to input detection matrix, and sets
 *      destination address of output EDMA to range profile.
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 *  @param[in] cfarHwaObj Pointer to CFAR DPU instance
 *  @param[in] detMatrix Pointer to input detection matrix
 *  @param[in] rangeProfile Pointer to output range profile
 *
 *
 *  @retval EDMA error code, see EDMA API.
 */
int32_t CFAR_DET_HWA_prepareEdmaRunTime(CFARHwaObj     *cfarHwaObj,
                                        DPIF_DetMatrix *detMatrix,
                                        uint32_t       *rangeProfile)
{
    int32_t                       retVal = 0;
    DPU_CFARProcHWA_HW_Resources *pRes   = &cfarHwaObj->res;

    /* Set Source Address */
    retVal = DPEDMA_setSourceAddress(pRes->edmaHandle,
                                     (uint8_t)cfarHwaObj->res.edmaHwaIn.channel,
                                     (uint32_t)SOC_virtToPhy(detMatrix->data));
    if (retVal != 0)
    {
        goto exit;
    }

    /* Set Destination Address */
    retVal = DPEDMA_setDestinationAddress(pRes->edmaHandle,
                                          (uint8_t)cfarHwaObj->res.edmaHwaOut.channel,
                                          (uint32_t)SOC_virtToPhy(rangeProfile));
exit:
    return retVal;
}

/**
 *  @b Description
 *  @n  Convert range field of view meters to indices
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  fovRangeCfg Field of view configuration for range (units in meters)
 *
 * @param[in]  numRangeBins umber of range bins
 *
 * @param[in]  rangeStep Range resolution (range bin to meter conversion)
 *
 * @param[in]  rangeBias Range bias (m)
 *
 * @param[out]  detectionCfg Field of view configuration for range (range indices)
 *
 */
void CFAR_DET_HWA_ConfigRangeView(DPU_CFARProc_RangeFovCfg  *fovRangeCfg,
                                  uint16_t                   numRangeBins,
                                  float                      rangeStep,
                                  float                      rnageBias,
                                  DPU_CFARProc_detectionCfg *detectionCfg)
{
    int16_t minIdx = (int16_t)((fovRangeCfg->min + rnageBias) / rangeStep + 0.5);
    int16_t maxIdx = (int16_t)((fovRangeCfg->max + rnageBias) / rangeStep + 0.5);

    if (minIdx < 0)
    {
        minIdx = 0;
    }
    if (minIdx > (int16_t)numRangeBins - 1)
    {
        minIdx = numRangeBins - 1;
    }

    if (maxIdx < 0)
    {
        maxIdx = 0;
    }
    if (maxIdx > (int16_t)numRangeBins - 1)
    {
        maxIdx = numRangeBins - 1;
    }

    detectionCfg->skipLeftRange  = minIdx;
    detectionCfg->skipRightRange = (numRangeBins - 1) - maxIdx;
}

#define PI_OVER_180 0.01745329252
/**
 *  @b Description
 *  @n  Convert angle Fov to number of skipped indices.
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  angle Angle in radians
 *
 * @param[in]  fftSize Angle FFT size
 *
 * @param[in]  skipFromLeft true: return number of skipped bins from left, false: return number of skipped bins from right
 *
 * @param[out]  skipBins Number of skipped bins from left or right side
 *
 */
int16_t CFAR_DET_HWA_ConvertFovToIdx(float angle, uint16_t fftSize, bool skipFromLeft)
{

    float   temp;
    int32_t ind;
    int16_t numSkippedBins;

    temp = fftSize / 2 * sin(angle * PI_OVER_180);
    if (temp < 0)
    {
        ind = (int32_t)(temp - 0.5);
    }
    else
    {
        ind = (int32_t)(temp + 0.5);
    }

    if (skipFromLeft)
    {
        numSkippedBins = fftSize / 2 + ind;
        if (numSkippedBins < 1)
        {
            numSkippedBins = 1;
        }
    }
    else
    {
        numSkippedBins = fftSize / 2 - 1 - ind;
        if (numSkippedBins < 0)
        {
            numSkippedBins = 0;
        }
    }

    return numSkippedBins;
}

/**
 *  @b Description
 *  @n  Convert angle field of view meters to indices
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  fovRangeCfg Field of view configuration for range (units in meters)
 *
 * @param[in]  numRangeBins umber of range bins
 *
 * @param[in]  rangeStep Range resolution (range bin to meter conversion)
 *
 * @param[out]  detectionCfg Field of view configuration for range (range indices)
 *
 */
void CFAR_DET_HWA_ConfigAoaFov(DPU_CFARProc_AoaFovCfg    *fovAoaCfg,
                               uint16_t                   azimuthFftSize,
                               uint16_t                   elevationFftSize,
                               DPU_CFARProc_detectionCfg *detectionCfg)
{

    detectionCfg->skipLeftAzim  = CFAR_DET_HWA_ConvertFovToIdx(fovAoaCfg->minAzimuthDeg, azimuthFftSize, true);
    detectionCfg->skipRightAzim = CFAR_DET_HWA_ConvertFovToIdx(fovAoaCfg->maxAzimuthDeg, azimuthFftSize, false);

    // if no rangeCompensation is specified, make sure we don't miss the zero-index of the FFT.
    if (rangeCompCfg.minCompAngle1 <= MIN_ANGLE_IN_FOV)
    {
        detectionCfg->rangeCompLeftStart1 = 0;
    }
    else
    {
        detectionCfg->rangeCompLeftStart1 = CFAR_DET_HWA_ConvertFovToIdx(rangeCompCfg.minCompAngle1, azimuthFftSize, true);
    }

    if (rangeCompCfg.minCompAngle2 <= MIN_ANGLE_IN_FOV)
    {
        detectionCfg->rangeCompLeftStart2 = 0;
    }
    else
    {
        detectionCfg->rangeCompLeftStart2 = CFAR_DET_HWA_ConvertFovToIdx(rangeCompCfg.minCompAngle2, azimuthFftSize, true);
    }

    // if no rangeCompensation is specified, make sure we don't miss the final index of the FFT.
    if (rangeCompCfg.maxCompAngle1 > 89)
    {
        detectionCfg->rangeCompRightEnd1 = 0;
    }
    else
    {
        detectionCfg->rangeCompRightEnd1 = CFAR_DET_HWA_ConvertFovToIdx(rangeCompCfg.maxCompAngle1, azimuthFftSize, false);
    }
    // if no rangeCompensation is specified, make sure we don't miss the final index of the FFT.
    if (rangeCompCfg.maxCompAngle2 > 89)
    {
        detectionCfg->rangeCompRightEnd2 = 0;
    }
    else
    {
        detectionCfg->rangeCompRightEnd2 = CFAR_DET_HWA_ConvertFovToIdx(rangeCompCfg.maxCompAngle2, azimuthFftSize, false);
    }

    detectionCfg->secondaryCompSNRDrop = rangeCompCfg.snrDropfromAngle1ToAngle2;
    detectionCfg->skipLeftElev         = CFAR_DET_HWA_ConvertFovToIdx(fovAoaCfg->minElevationDeg, elevationFftSize, true);
    detectionCfg->skipRightElev        = CFAR_DET_HWA_ConvertFovToIdx(fovAoaCfg->maxElevationDeg, elevationFftSize, false);
}


#if 1
/**
 *  @b Description
 *  @n  Saves configuration parameters to CFAR instance
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  cfarHwaObj Pointer to CFAR instance
 *
 * @param[in]  cfarHwaCfg Pointer to configuration
 *
 *
 */
void CFAR_DET_HWA_saveConfiguration(CFARHwaObj             *cfarHwaObj,
                                    DPU_CFARProcHWA_Config *cfarHwaCfg)
{
    cfarHwaObj->staticCfg       = cfarHwaCfg->staticCfg;
    cfarHwaObj->res             = cfarHwaCfg->res;
    cfarHwaObj->cfarCfg         = *cfarHwaCfg->dynCfg.cfarCfg;
    cfarHwaObj->cfarScndPassCfg = *cfarHwaCfg->dynCfg.cfarScndPassCfg;
    cfarHwaObj->staticCfg       = cfarHwaCfg->staticCfg;

    // cfarHwaObj->hwaParamsSaveLoc = cfarHwaCfg->res.hwaCfg.hwaParamsSaveLoc; //ToDo Cleanup this, redundant


    CFAR_DET_HWA_ConfigRangeView(cfarHwaCfg->dynCfg.fovRange,
                                 cfarHwaObj->staticCfg.numRangeBins,
                                 cfarHwaObj->staticCfg.rangeStep,
                                 cfarHwaObj->staticCfg.rangeBias,
                                 &cfarHwaObj->detectionCfg);

    CFAR_DET_HWA_ConfigAoaFov(cfarHwaCfg->dynCfg.fovAoaCfg,
                              cfarHwaObj->staticCfg.azimuthFftSize,
                              cfarHwaObj->staticCfg.elevationFftSize,
                              &cfarHwaObj->detectionCfg);
}

#endif


/**
 *  @b Description
 *  @n  Triggers CFAR execution.
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  cfarHwaObj Pointer to CFAR instance
 *
 */
int32_t CFAR_DET_HWA_triggerCFAR(CFARHwaObj *cfarHwaObj)
{
    uint32_t                      baseAddr, regionId;
    int32_t                       retVal = 0;
    DPU_CFARProcHWA_HW_Resources *pRes   = &cfarHwaObj->res;

    baseAddr = EDMA_getBaseAddr(pRes->edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(pRes->edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    /* Enable the HWA */
    retVal = HWA_enable(cfarHwaObj->hwaHandle, 1);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Trigger EDMA  + CFAR */
    retVal = EDMAEnableTransferRegion(
        baseAddr,
        regionId,
        pRes->edmaHwaIn.channel,
        EDMA_TRIG_MODE_MANUAL); // EDMA_TRIG_MODE_EVENT

    if (retVal != 1)
    {
        retVal = -1;
        goto exit;
    }
    else
    {
        retVal = 0;
    }

    /*retVal  = EDMA_startTransfer(pRes->edmaHandle, pRes->edmaHwaIn.channel, EDMA_CHANNEL_TYPE_DMA);
    if (retVal != EDMA_NO_ERROR)
    {
        goto exit;
    }*/
exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *      Converts HWA CFAR Detection list format into bit mask format:
 *      word = (rngInd*dopFftsize+dopInd) >> 5
 *      bit = (rngInd*dopFftsize+dopInd) & 31
 *
 *  \ingroup    DPU_CFARCAPROC_INTERNAL_FUNCTION
 *
 *
 * @param[in]  hwaCfarDetList Pointer to HWA CFAR detection list
 *
 * @param[in]  numDopBins Number of Doppler bins (must be power of 2)
 *
 * @param[in]  numCfarDetections Number of detected points reported by CFAR
 *
 * @param[in]  cfarDomain 0: CFAR in range domain, 1: CFAR in Doppler domain
 *
 * @param[out]  cfarDetOutBitMask Pointer to output bit mask (compressed list)
 *
 * @param[in]  isDetMatLog2 true: matrix is log2 in Q11 format with element size uint16, false: matrix is linear with  elements size: uint32_t
 *
 *
 */
void CFARHWA_convHwaCfarDetListToDetMask(DPU_CFARProcHWA_CfarDetOutput *hwaCfarDetList,
                                         uint32_t                       numDopBins,
                                         uint32_t                       numCfarDetections,
                                         uint32_t                       cfarDomain,
                                         uint32_t                       cfarDetOutBitMaskSizeBytes,
                                         uint32_t                      *cfarDetOutBitMask,
                                         void                          *noiseMatrix,
                                         bool                           isDetMatLog2)
{
    uint32_t  i;
    uint32_t  bit, word;
    uint32_t  temp;
    uint32_t  cellOffset;
    uint32_t  iterOffset;
    uint32_t *noiseOutArray     = (uint32_t *)noiseMatrix;
    uint16_t *noiseOutArrayLog2 = (uint16_t *)noiseMatrix;

    if (cfarDomain == DPU_CFAR_RANGE_DOMAIN)
    {
        /*CFAR in Range domain, cellIdx is range index, iterNum is Doppler index*/
        cellOffset = numDopBins;
        iterOffset = 1;
    }
    else
    {
        /*CFAR in Doppler domain, cellIdx is Doppler index, iterNum is Range index*/
        cellOffset = 1;
        iterOffset = numDopBins;
    }

    /* Zero bit-mask */
    memset(cfarDetOutBitMask, 0, cfarDetOutBitMaskSizeBytes);

    for (i = 0; i < numCfarDetections; i++)
    {
        temp = hwaCfarDetList[i].cellIdx * cellOffset + hwaCfarDetList[i].iterNum * iterOffset;
        word = temp >> 5;
        bit  = temp & 31;
        cfarDetOutBitMask[word] |= 1 << bit;
        if (isDetMatLog2)
        {
            noiseOutArrayLog2[hwaCfarDetList[i].cellIdx * numDopBins + hwaCfarDetList[i].iterNum] = hwaCfarDetList[i].noise;
        }
        else
        {
            noiseOutArray[hwaCfarDetList[i].cellIdx * numDopBins + hwaCfarDetList[i].iterNum] = hwaCfarDetList[i].noise;
        }
    }
}


/**
 *  @b Description
 *  @n
 *
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 * @param[in]  cfarHwaObj CFAR object
 *
 * @param[in]  detMatrix Detection matrix
 *
 * @param[in]  inParams Input parameters
 *
 * @param[OUT]  outParams Output results
 *
 */
uint32_t CFAR_DET_HWA_cfarRangeDoppler_ProcList(CFARHwaObj                  *cfarHwaObj,
                                                DPIF_DetMatrix              *detMatrix,
                                                DPU_CFARProcHWA_InputParams *inParams,
                                                DPU_CFARProcHWA_OutParams   *outParams)
{
    uint32_t  numCfarDet;
    uint32_t  numCfarRangeDet       = cfarHwaObj->numHwaCfarObjs;
    uint32_t *detMat                = detMatrix->data;
    uint16_t *detMatLog2            = (uint16_t *)detMatrix->data;
    uint16_t  log2NumDopplerBins    = cfarHwaObj->staticCfg.log2NumDopplerBins;
    uint16_t  numDopplerBins        = cfarHwaObj->staticCfg.numDopplerBins;
    uint16_t  numDopplerBinsBitMask = numDopplerBins - 1;

    uint32_t cfarDetectionListSize = outParams->detObjOutMaxSize;

    DPU_CFARProcHWA_CfarDetOutput *cfarRangeDetOutList = cfarHwaObj->res.hwaMemOutDetList;

    DPIF_CFARRngDopDetListElement *detRngDopList = outParams->detRngDopList;

    int32_t  rangeIdx;
    uint32_t cfarCellNoise;
    int32_t  dopplerSgnIdx;
    int32_t  dopplerIdx;
    float    radialVelocity;
    float    snrdB, noisedB, range;

    int32_t selRangeMin = cfarHwaObj->detectionCfg.skipLeftRange;
    int32_t selRangeMax = cfarHwaObj->staticCfg.numRangeBins - cfarHwaObj->detectionCfg.skipRightRange - 1;

    float    dopplerBinOffset, rangeBinOffset;
    uint32_t dopplerFftMask = numDopplerBins - 1;
    int32_t  dopplerLeftInd, dopplerRightInd;

    uint32_t *noiseMatrix     = (uint32_t *)cfarHwaObj->res.hwaMemInp; // Temporary as scratch to save noise
    uint16_t *noiseMatrixLog2 = (uint16_t *)cfarHwaObj->res.hwaMemInp; // Temporary as scratch to save noise

    uint32_t word;
    uint32_t wordInd, bitInd;
    uint32_t numWords = (cfarHwaObj->staticCfg.numDopplerBins * cfarHwaObj->staticCfg.numRangeBins + 31) / 32;
    uint32_t linBitInd;
    bool     isDetMatLog2 = cfarHwaObj->staticCfg.isDetMatrixLogScale;

    /* Generate range/doppler bit-mask */
    CFARHWA_convHwaCfarDetListToDetMask(cfarRangeDetOutList,
                                        numDopplerBins,
                                        numCfarRangeDet,
                                        DPU_CFAR_RANGE_DOMAIN,
                                        numWords * sizeof(uint32_t),
                                        cfarHwaObj->res.rangeDopplerDetPointBitMap,
                                        (void *)noiseMatrix,
                                        isDetMatLog2);
    if (!cfarHwaObj->cfarScndPassCfg.enabled)
    {
        numCfarDet = 0;
        for (wordInd = 0; wordInd < numWords; wordInd++)
        {
            word = cfarHwaObj->res.rangeDopplerDetPointBitMap[wordInd];
            for (bitInd = 0; bitInd < 32; bitInd++)
            {
                if (!((word >> bitInd) & 0x1))
                {
                    continue;
                }

                linBitInd  = wordInd * 32 + bitInd;
                dopplerIdx = linBitInd & numDopplerBinsBitMask;
                rangeIdx   = linBitInd >> log2NumDopplerBins;
                if (!isDetMatLog2)
                {
                    cfarCellNoise = noiseMatrix[rangeIdx * numDopplerBins + dopplerIdx];
                }
                else
                {
                    cfarCellNoise = noiseMatrixLog2[rangeIdx * numDopplerBins + dopplerIdx];
                }

                /* Check if it is inside range FOV */
                if ((rangeIdx < selRangeMin) || (rangeIdx > selRangeMax))
                {
                    continue;
                }

                if (rangeCompCfg.enabled)
                {
                    // compute snrdB and noisedB earlier to avoid unnecessary calculations if point is to be removed
                    noisedB = 20 * log10((float)cfarCellNoise);
                    snrdB   = 20 * log10((float)detMat[rangeIdx * numDopplerBins + dopplerIdx]) - noisedB;
                    if (snrdB < CFAR_DET_HWA_RangeComp_SNRNeededToKeepPoint((float)rangeIdx, 0))
                    {
                        continue;
                    }
                }

                dopplerBinOffset = 0.;
                rangeBinOffset   = 0.;

                /* Check for local peak in range domain */
                if (cfarHwaObj->cfarCfg.enableLocalMaxRange)
                {
                    int32_t rangeLeftInd, rangeRightInd;
                    rangeLeftInd  = rangeIdx > selRangeMin ? rangeIdx - 1 : rangeIdx;
                    rangeRightInd = rangeIdx < selRangeMax ? rangeIdx + 1 : rangeIdx;
                    if (!isDetMatLog2)
                    {
                        if (!((detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeLeftInd * numDopplerBins + dopplerIdx]) &&
                              (detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeRightInd * numDopplerBins + dopplerIdx])))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (!((detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeLeftInd * numDopplerBins + dopplerIdx]) &&
                              (detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeRightInd * numDopplerBins + dopplerIdx])))
                        {
                            continue;
                        }
                    }
                }

                dopplerLeftInd  = (dopplerIdx - 1) & dopplerFftMask;
                dopplerRightInd = (dopplerIdx + 1) & dopplerFftMask;

                /* Check for local peak in Doppler domain */
                if (cfarHwaObj->cfarCfg.enableLocalMaxDoppler)
                {
                    if (!isDetMatLog2)
                    {
                        /*If this is local peak in Doppler domain */
                        if (!((detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeIdx * numDopplerBins + dopplerLeftInd]) &&
                              (detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeIdx * numDopplerBins + dopplerRightInd])))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        /*If this is local peak in Doppler domain */
                        if (!((detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeIdx * numDopplerBins + dopplerLeftInd]) &&
                              (detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeIdx * numDopplerBins + dopplerRightInd])))
                        {
                            continue;
                        }
                    }
                }

                if (!isDetMatLog2)
                {
                    /* Interpolate in range domain */
                    if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
                    {
                        if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                        {
                            float P        = 1.36; // Hanning window in range FFT
                            float xLeft    = detMat[(rangeIdx - 1) * numDopplerBins + dopplerIdx];
                            float xRight   = detMat[(rangeIdx + 1) * numDopplerBins + dopplerIdx];
                            float x        = detMat[rangeIdx * numDopplerBins + dopplerIdx];
                            rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                        }
                    }

                    /* Interpolate in Doppler domain */
                    if (cfarHwaObj->cfarCfg.enableInterpDopplerDom)
                    {
                        float P          = 1.36; // Hanning window in Doppler FFT
                        float xLeft      = detMat[rangeIdx * numDopplerBins + dopplerLeftInd];
                        float xRight     = detMat[rangeIdx * numDopplerBins + dopplerRightInd];
                        float x          = detMat[rangeIdx * numDopplerBins + dopplerIdx];
                        dopplerBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                    }
                }
                else
                {
                    /* Interpolate in range domain */
                    if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
                    {
                        if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                        {
                            float P        = 1.36; // Hanning window in range FFT
                            float xLeft    = powf(2, detMatLog2[(rangeIdx - 1) * numDopplerBins + dopplerIdx] / 2048.);
                            float xRight   = powf(2, detMatLog2[(rangeIdx + 1) * numDopplerBins + dopplerIdx] / 2048.);
                            float x        = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] / 2048.);
                            rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                        }
                    }

                    /* Interpolate in Doppler domain */
                    if (cfarHwaObj->cfarCfg.enableInterpDopplerDom)
                    {
                        float P          = 1.36; // Hanning window in Doppler FFT
                        float xLeft      = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerLeftInd] / 2048.);
                        float xRight     = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerRightInd] / 2048.);
                        float x          = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] / 2048.);
                        dopplerBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                    }
                }


                if (!isDetMatLog2)
                {
                    noisedB = 20 * log10f((float)cfarCellNoise);
                    snrdB   = 20 * log10f((float)detMat[rangeIdx * numDopplerBins + dopplerIdx]) - noisedB;
                }
                else
                {
                    noisedB = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)cfarCellNoise);
                    snrdB   = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)detMatLog2[rangeIdx * numDopplerBins + dopplerIdx]) - noisedB;
                }

                dopplerSgnIdx = (int32_t)dopplerIdx;
                if (dopplerSgnIdx >= (int32_t)(numDopplerBins >> 1))
                {
                    dopplerSgnIdx = dopplerSgnIdx - (int32_t)numDopplerBins;
                }
                radialVelocity = cfarHwaObj->staticCfg.dopplerStep * (dopplerSgnIdx + dopplerBinOffset);

                range = (rangeIdx + rangeBinOffset) * cfarHwaObj->staticCfg.rangeStep;

                /* Subtract range bias */
                range -= cfarHwaObj->staticCfg.rangeBias;
                if (range < 0)
                {
                    range = 0;
                }

                detRngDopList[numCfarDet].range      = range;
                detRngDopList[numCfarDet].doppler    = radialVelocity;
                detRngDopList[numCfarDet].rangeIdx   = (uint16_t)rangeIdx;
                detRngDopList[numCfarDet].dopplerIdx = (int16_t)dopplerSgnIdx;
                detRngDopList[numCfarDet].noisedB    = noisedB;
                detRngDopList[numCfarDet].snrdB      = snrdB;

                numCfarDet++;
                if (numCfarDet >= cfarDetectionListSize)
                {
                    goto exit;
                }
            }
        }
    }
    else
    {
        uint32_t listInd;
        numCfarDet = 0;
        for (listInd = 0; listInd < cfarHwaObj->numHwaCfarScndPassObjs; listInd++)
        {
            DPU_CFARProcHWA_CfarDetOutput *scndPassList = (DPU_CFARProcHWA_CfarDetOutput *)cfarHwaObj->res.hwaMemOutRangeProfile;

            dopplerIdx = scndPassList[listInd].cellIdx;
            rangeIdx   = scndPassList[listInd].iterNum;

            /* Check the point in range-CFAR detection bit map */
            linBitInd = dopplerIdx + rangeIdx * numDopplerBins;
            wordInd   = linBitInd >> 5;
            bitInd    = linBitInd & 31;
            if (!((cfarHwaObj->res.rangeDopplerDetPointBitMap[wordInd] >> bitInd) & 0x1))
            {
                continue;
            }

            if (isDetMatLog2)
            {
                cfarCellNoise = noiseMatrixLog2[rangeIdx * numDopplerBins + dopplerIdx];
            }
            else
            {
                cfarCellNoise = noiseMatrix[rangeIdx * numDopplerBins + dopplerIdx];
            }


            /* Check if it is inside range FOV */
            if ((rangeIdx < selRangeMin) || (rangeIdx > selRangeMax))
            {
                continue;
            }

            dopplerBinOffset = 0.;
            rangeBinOffset   = 0.;

            /* Check for local peak in range domain */
            if (cfarHwaObj->cfarCfg.enableLocalMaxRange)
            {
                int32_t rangeLeftInd, rangeRightInd;
                rangeLeftInd  = rangeIdx > selRangeMin ? rangeIdx - 1 : rangeIdx;
                rangeRightInd = rangeIdx < selRangeMax ? rangeIdx + 1 : rangeIdx;
                if (!isDetMatLog2)
                {
                    if (!((detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeLeftInd * numDopplerBins + dopplerIdx]) &&
                          (detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeRightInd * numDopplerBins + dopplerIdx])))
                    {
                        continue;
                    }
                }
                else
                {
                    if (!((detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeLeftInd * numDopplerBins + dopplerIdx]) &&
                          (detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeRightInd * numDopplerBins + dopplerIdx])))
                    {
                        continue;
                    }
                }
            }

            dopplerLeftInd  = (dopplerIdx - 1) & dopplerFftMask;
            dopplerRightInd = (dopplerIdx + 1) & dopplerFftMask;

            /* Check for local peak in Doppler domain */
            if (cfarHwaObj->cfarCfg.enableLocalMaxDoppler)
            {
                if (!isDetMatLog2)
                {
                    /*If this is local peak in azimuth domain */
                    if (!((detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeIdx * numDopplerBins + dopplerLeftInd]) &&
                          (detMat[rangeIdx * numDopplerBins + dopplerIdx] >= detMat[rangeIdx * numDopplerBins + dopplerRightInd])))
                    {
                        continue;
                    }
                }
                else
                {
                    /*If this is local peak in azimuth domain */
                    if (!((detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeIdx * numDopplerBins + dopplerLeftInd]) &&
                          (detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] >= detMatLog2[rangeIdx * numDopplerBins + dopplerRightInd])))
                    {
                        continue;
                    }
                }
            }

            if (!isDetMatLog2)
            {
                /* Interpolate in range domain */
                if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
                {
                    if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                    {
                        float P        = 1.36; // Hanning window in range FFT
                        float xLeft    = detMat[(rangeIdx - 1) * numDopplerBins + dopplerIdx];
                        float xRight   = detMat[(rangeIdx + 1) * numDopplerBins + dopplerIdx];
                        float x        = detMat[rangeIdx * numDopplerBins + dopplerIdx];
                        rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                    }
                }

                /* Interpolate in Doppler domain */
                if (cfarHwaObj->cfarCfg.enableInterpDopplerDom)
                {
                    float P          = 1.36; // Hanning window in Doppler FFT
                    float xLeft      = detMat[rangeIdx * numDopplerBins + dopplerLeftInd];
                    float xRight     = detMat[rangeIdx * numDopplerBins + dopplerRightInd];
                    float x          = detMat[rangeIdx * numDopplerBins + dopplerIdx];
                    dopplerBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                }
            }
            else
            {
                /* Interpolate in range domain */
                if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
                {
                    if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                    {
                        float P        = 1.36; // Hanning window in range FFT
                        float xLeft    = powf(2, detMatLog2[(rangeIdx - 1) * numDopplerBins + dopplerIdx] / 2048.);
                        float xRight   = powf(2, detMatLog2[(rangeIdx + 1) * numDopplerBins + dopplerIdx] / 2048.);
                        float x        = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] / 2048.);
                        rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                    }
                }

                /* Interpolate in Doppler domain */
                if (cfarHwaObj->cfarCfg.enableInterpDopplerDom)
                {
                    float P          = 1.36; // Hanning window in Doppler FFT
                    float xLeft      = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerLeftInd] / 2048.);
                    float xRight     = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerRightInd] / 2048.);
                    float x          = powf(2, detMatLog2[rangeIdx * numDopplerBins + dopplerIdx] / 2048.);
                    dopplerBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                }
            }

            if (!isDetMatLog2)
            {
                noisedB = 20 * log10f((float)cfarCellNoise);
                snrdB   = 20 * log10f((float)detMat[rangeIdx * numDopplerBins + dopplerIdx]) - noisedB;
            }
            else
            {
                noisedB = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)cfarCellNoise);
                snrdB   = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)detMatLog2[rangeIdx * numDopplerBins + dopplerIdx]) - noisedB;
            }

            dopplerSgnIdx = (int32_t)dopplerIdx;
            if (dopplerSgnIdx >= (int32_t)(numDopplerBins >> 1))
            {
                dopplerSgnIdx = dopplerSgnIdx - (int32_t)numDopplerBins;
            }
            radialVelocity = cfarHwaObj->staticCfg.dopplerStep * (dopplerSgnIdx + dopplerBinOffset);

            range = (rangeIdx + rangeBinOffset) * cfarHwaObj->staticCfg.rangeStep;

            /* Subtract range bias */
            range -= cfarHwaObj->staticCfg.rangeBias;
            if (range < 0)
            {
                range = 0;
            }

            detRngDopList[numCfarDet].range      = range;
            detRngDopList[numCfarDet].doppler    = radialVelocity;
            detRngDopList[numCfarDet].rangeIdx   = (uint16_t)rangeIdx;
            detRngDopList[numCfarDet].dopplerIdx = (int16_t)dopplerSgnIdx;
            detRngDopList[numCfarDet].noisedB    = noisedB;
            detRngDopList[numCfarDet].snrdB      = snrdB;

            numCfarDet++;
            if (numCfarDet >= cfarDetectionListSize)
            {
                goto exit;
            }
        }
    }
exit:
    return numCfarDet;
}


/*
float azimBinCorrArray[64] = {
      2.2227, 1.7450, 1.5949, 1.5176, 1.1565, 0.5950, 0.3515, 0.7943,
      1.6760, 2.3317, 2.3478, 1.9249, 1.5369, 1.3877, 1.2877, 1.0162,
      0.5916, 0.1884, -0.1188, -0.3850, -0.6645, -0.8811, -0.9000, -0.6762,
      -0.2963, 0.0694, 0.3072, 0.4048, 0.3690, 0.1991, -0.0853, -0.3960,
      -0.6866, -0.9888, -1.3330, -1.6456, -1.8773, -2.1389, -2.5436, -2.9702,
      -3.1593, -3.1001, -3.0024, -2.8156, -2.0361, -0.3178, 1.8172, 3.2699,
      3.4439, 2.8329, 2.3950, 2.5288, 2.8204, 2.7344, 2.2722, 1.8665,
      1.7865, 1.8250, 1.6529, 1.3248, 1.2701, 1.7545, 2.4591, 2.7822
};
*/

int8_t azimBinCorrArrayQ4[] = {
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45,
    36,
    28,
    26,
    24,
    19,
    10,
    6,
    13,
    27,
    37,
    38,
    31,
    25,
    22,
    21,
    16,
    9,
    3,
    -2,
    -6,
    -11,
    -14,
    -14,
    -11,
    -5,
    1,
    5,
    6,
    6,
    3,
    -1,
    -6,
    -11,
    -16,
    -21,
    -26,
    -30,
    -34,
    -41,
    -48,
    -51,
    -50,
    -48,
    -45,
    -33,
    -5,
    29,
    52,
    55,
    45,
    38,
    40,
    45,
    44,
    36,
    30,
    29,
    29,
    26,
    21,
    20,
    28,
    39,
    45
};

int8_t elevBinCorrArrayQ4[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


/**
 *  @b Description
 *  @n
 *
 *
 *  \ingroup    DPU_CFARPROC_INTERNAL_FUNCTION
 *
 * @param[in]  cfarHwaObj CFAR object
 *
 * @param[in]  detMatrix Detection matrix
 *
 * @param[in]  inParams Input parameters
 *
 * @param[OUT]  outParams Output results
 *
 */
uint32_t CFAR_DET_HWA_cfarRange_ProcList(CFARHwaObj                  *cfarHwaObj,
                                         DPIF_DetMatrix              *detMatrix,
                                         DPU_CFARProcHWA_InputParams *inParams,
                                         DPU_CFARProcHWA_OutParams   *outParams)
{
    uint32_t  numCfarDet;
    uint32_t  numCfarRangeDet       = cfarHwaObj->numHwaCfarObjs;
    uint32_t *detMat                = detMatrix->data;
    uint16_t  azimuthFftSize        = cfarHwaObj->staticCfg.azimuthFftSize;
    uint16_t  elevationFftSize      = cfarHwaObj->staticCfg.elevationFftSize;
    uint16_t  numDopplerBins        = cfarHwaObj->staticCfg.numDopplerBins;
    uint32_t  cfarDetectionListSize = outParams->detObjOutMaxSize;

    DPU_CFARProcHWA_CfarDetOutput *cfarRangeDetOutList = cfarHwaObj->res.hwaMemOutDetList;
    // ToDo this is pointing to HWA memory
    // DPU_CFARProcHWA_HwaMaxOutput * maxAzimuthArray = (DPU_CFARProcHWA_HwaMaxOutput *) (((uint32_t) cfarRangeDetOutList) + CFAR_HWA_MEM_OFFSET_FOR_MAX_AZIMUTH_PER_RNG_BIN);
    // ToDo this is pointing to local memory
    uint32_t *maxAzimuthArray = outParams->rangeProfile;

    uint8_t *dopplerIndexMatrix   = cfarHwaObj->res.dopplerIndexMatrix.data;
    uint8_t *elevationIndexMatrix = cfarHwaObj->res.elevationIndexMatrix.data;

    DPIF_PointCloudCartesianExt      *detObjOut    = outParams->detObjOut;
    DPIF_PointCloudRngAzimElevDopInd *detObjIndOut = outParams->detObjIndOut;

    uint32_t rangeIdx;
    uint32_t azimuthIdx;
    uint32_t i;
    uint32_t cfarCellNoise;
    int32_t  dopplerSgnIdx;
    int32_t  azimuthSgnIdx;
    int32_t  elevationSgnIdx;
    int32_t  dopplerIdx;
    float    radialVelocity;
    int32_t  elevationIdx;
    float    snrdB, noisedB, range;
    float    x, y, z;
    float    Wx, Wz;
    float    temp;
    int32_t  sideLobeThreshold;

    int32_t selRangeMin = cfarHwaObj->detectionCfg.skipLeftRange;
    int32_t selRangeMax = cfarHwaObj->staticCfg.numRangeBins - cfarHwaObj->detectionCfg.skipRightRange - 1;

    int32_t  selAzimMin = cfarHwaObj->detectionCfg.skipLeftAzim;
    int32_t  selAzimMax = cfarHwaObj->staticCfg.azimuthFftSize - cfarHwaObj->detectionCfg.skipRightAzim - 1;
    float    azimBinOffset, rangeBinOffset;
    uint32_t azimFftMask = cfarHwaObj->staticCfg.azimuthFftSize - 1;
    int32_t  azimLeftInd, azimRightInd;

    int32_t compAngleMin1 = cfarHwaObj->detectionCfg.rangeCompLeftStart1;
    int32_t compAngleMax1 = cfarHwaObj->staticCfg.azimuthFftSize - cfarHwaObj->detectionCfg.rangeCompRightEnd1;

    int32_t compAngleMin2 = cfarHwaObj->detectionCfg.rangeCompLeftStart2;
    int32_t compAngleMax2 = cfarHwaObj->staticCfg.azimuthFftSize - cfarHwaObj->detectionCfg.rangeCompRightEnd2;


    int32_t selElevMin;
    int32_t selElevMax;
    int32_t numOutputDopplerBins;

    float lambdaOverDistX = cfarHwaObj->staticCfg.lambdaOverDistX;
    float lambdaOverDistZ = cfarHwaObj->staticCfg.lambdaOverDistZ;

    float azimBinCorr, azimuthSgnBin;
    float elevBinCorr;
    float azimuthFullFftSize = (float)(cfarHwaObj->staticCfg.azimuthFftSize);
    float azimuthHalfFftSize = (float)(cfarHwaObj->staticCfg.azimuthFftSize >> 1);


    if (cfarHwaObj->staticCfg.angleDimension == 2)
    {
        selElevMin = cfarHwaObj->detectionCfg.skipLeftElev;
        selElevMax = cfarHwaObj->staticCfg.elevationFftSize - cfarHwaObj->detectionCfg.skipRightElev - 1;
    }

    if (cfarHwaObj->staticCfg.isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins = cfarHwaObj->staticCfg.numDopplerBins - 1;
    }
    else
    {
        numOutputDopplerBins = cfarHwaObj->staticCfg.numDopplerBins;
    }


    numCfarDet = 0;

    for (i = 0; i < numCfarRangeDet; i++)
    {
        rangeIdx      = cfarRangeDetOutList[i].cellIdx;
        azimuthIdx    = cfarRangeDetOutList[i].iterNum;
        cfarCellNoise = cfarRangeDetOutList[i].noise;

        /* Check if it is inside azimuth and range FOV */
        if ((rangeIdx >= selRangeMin) && (rangeIdx <= selRangeMax) &&
            (azimuthIdx >= selAzimMin) && (azimuthIdx <= selAzimMax))
        {
            azimBinOffset  = 0.;
            rangeBinOffset = 0.;

            /* Check for local peak in range domain */
            if (cfarHwaObj->cfarCfg.enableLocalMaxRange)
            {
                int32_t rangeLeftInd, rangeRightInd;
                rangeLeftInd  = rangeIdx > selRangeMin ? rangeIdx - 1 : rangeIdx;
                rangeRightInd = rangeIdx < selRangeMax ? rangeIdx + 1 : rangeIdx;
                if (!((detMat[rangeIdx * azimuthFftSize + azimuthIdx] >= detMat[rangeLeftInd * azimuthFftSize + azimuthIdx]) &&
                      (detMat[rangeIdx * azimuthFftSize + azimuthIdx] >= detMat[rangeRightInd * azimuthFftSize + azimuthIdx])))
                {
                    continue;
                }
            }


            azimLeftInd  = (azimuthIdx - 1) & azimFftMask;
            azimRightInd = (azimuthIdx + 1) & azimFftMask;

            /* Check for local peak in azimuth domain */
            if (cfarHwaObj->cfarCfg.enableLocalMaxAzimuth)
            {

                /*If this is local peak in azimuth domain */
                if (!((detMat[rangeIdx * azimuthFftSize + azimuthIdx] >= detMat[rangeIdx * azimuthFftSize + azimLeftInd]) &&
                      (detMat[rangeIdx * azimuthFftSize + azimuthIdx] >= detMat[rangeIdx * azimuthFftSize + azimRightInd])))
                {
                    continue;
                }
            }

            /*If peak is greater than side-lobe threshold*/
            sideLobeThreshold = (int32_t)((maxAzimuthArray[rangeIdx] * cfarHwaObj->cfarCfg.sideLobeThresholdScaleQ8) >> DPU_CFARPROCHWA_SHIFT_Q8);
            if (!(detMat[rangeIdx * azimuthFftSize + azimuthIdx] >= sideLobeThreshold))
            {
                continue;
            }

            if (rangeCompCfg.enabled)
            {
                // compute snrdB and noisedB earlier to avoid unnecessary calculations if point is to be removed
                noisedB = 20 * log10((float)cfarCellNoise);
                snrdB   = 20 * log10((float)detMat[rangeIdx * azimuthFftSize + azimuthIdx]) - noisedB;

                // If angle1 isn't enabled, then assume that it applies for the entire field of view
                if (azimuthIdx >= compAngleMin1 && azimuthIdx <= compAngleMax1)
                {
                    if (snrdB < CFAR_DET_HWA_RangeComp_SNRNeededToKeepPoint((float)rangeIdx, 0))
                    {
                        continue;
                    }
                }
                // If there is a secondaryCompSNRDrop enabled, then use it
                else if (cfarHwaObj->detectionCfg.secondaryCompSNRDrop > 0 && azimuthIdx >= compAngleMin2 && azimuthIdx <= compAngleMax2)
                {

                    if (snrdB < CFAR_DET_HWA_RangeComp_SNRNeededToKeepPoint((float)(rangeIdx), cfarHwaObj->detectionCfg.secondaryCompSNRDrop))
                    {
                        continue;
                    }
                }
            }

            /* Interpolate in range domain */
            if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
            {
                if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                {
                    float P        = 1.75; // Blackman window in range FFT
                    float xLeft    = detMat[(rangeIdx - 1) * azimuthFftSize + azimuthIdx];
                    float xRight   = detMat[(rangeIdx + 1) * azimuthFftSize + azimuthIdx];
                    float x        = detMat[rangeIdx * azimuthFftSize + azimuthIdx];
                    rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                }
            }

            /* Interpolate in azimuth domain */
            if (cfarHwaObj->cfarCfg.enableInterpAzimuthDom)
            {
                float xLeft   = detMat[rangeIdx * azimuthFftSize + azimLeftInd];
                float xRight  = detMat[rangeIdx * azimuthFftSize + azimRightInd];
                float x       = detMat[rangeIdx * azimuthFftSize + azimuthIdx];
                azimBinOffset = (xRight - xLeft) / (4. * x - 2. * xLeft - 2. * xRight);
            }

            if (cfarHwaObj->staticCfg.dopElevDimReductOrder == 0)
            {
                /* V1: Dimensionality reduction order: Doppler then Elevation */
                if (cfarHwaObj->staticCfg.angleDimension == 2)
                {
                    elevationIdx = elevationIndexMatrix[rangeIdx * azimuthFftSize + azimuthIdx];
                    if ((elevationIdx < selElevMin) || (elevationIdx > selElevMax))
                    {
                        continue;
                    }
                }
                else
                {
                    elevationIdx = (int32_t)(elevationFftSize >> 1);
                }

                if ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                    (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2))
                {
                    if (cfarHwaObj->staticCfg.angleDimension == 2)
                    {
                        dopplerIdx = dopplerIndexMatrix[rangeIdx * azimuthFftSize * elevationFftSize + elevationFftSize * azimuthIdx + elevationIdx];
                    }
                    else
                    {
                        dopplerIdx = dopplerIndexMatrix[rangeIdx * azimuthFftSize + azimuthIdx];
                    }
                }
                else
                {
                    dopplerIdx = 0;
                }
                if (cfarHwaObj->staticCfg.isStaticClutterRemovalEnabled &&
                    ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                     (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2)))
                {
                    dopplerIdx++;
                }
            }
            else
            {
                /* V2: Dimensionality reduction order: Elevation then Doppler */
                if ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                    (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2))
                {
                    dopplerIdx = dopplerIndexMatrix[rangeIdx * azimuthFftSize + azimuthIdx];
                }
                else
                {
                    dopplerIdx = 0;
                }

                if (cfarHwaObj->staticCfg.angleDimension == 2)
                {
                    elevationIdx = elevationIndexMatrix[rangeIdx * azimuthFftSize * numOutputDopplerBins + numOutputDopplerBins * azimuthFftSize + azimuthIdx];
                    if ((elevationIdx < selElevMin) || (elevationIdx > selElevMax))
                    {
                        continue;
                    }
                }
                else
                {
                    elevationIdx = (int32_t)(elevationFftSize >> 1);
                }


                if (cfarHwaObj->staticCfg.isStaticClutterRemovalEnabled &&
                    ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                     (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2)))
                {
                    dopplerIdx++;
                }
            }


            if (cfarHwaObj->staticCfg.isDetMatrixLogScale)
            {
                // noisedB = ((float) cellLog2NoiseQ11) * 6. /2048.;
                noisedB = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)cfarCellNoise);
                snrdB   = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)detMat[rangeIdx * azimuthFftSize + azimuthIdx]) - noisedB;
            }
            else
            {
                noisedB = 20 * log10f((float)cfarCellNoise);
                snrdB   = 20 * log10f((float)detMat[rangeIdx * azimuthFftSize + azimuthIdx]) - noisedB;
            }

            /* Azimuth/elevation correction */
            if (cfarHwaObj->cfarCfg.lookUpTableCorrectAzimuthDom)
            {
                azimBinCorr = (float)azimBinCorrArrayQ4[/*azimuthFftSize * elevationIdx*/ +azimuthIdx] * 0.0625;
                elevBinCorr = (float)elevBinCorrArrayQ4[/*azimuthFftSize * elevationIdx*/ +azimuthIdx] * 0.0625;
            }
            else
            {
                azimBinCorr = 0;
                elevBinCorr = 0;
            }

            azimuthSgnIdx   = (int32_t)azimuthIdx - (int32_t)(azimuthFftSize >> 1);
            elevationSgnIdx = (int32_t)elevationIdx - (int32_t)(elevationFftSize >> 1);

            dopplerSgnIdx = (int32_t)dopplerIdx;
            if (dopplerSgnIdx >= (int32_t)(numDopplerBins >> 1))
            {
                dopplerSgnIdx = dopplerSgnIdx - (int32_t)numDopplerBins;
            }
            radialVelocity = cfarHwaObj->staticCfg.dopplerStep * dopplerSgnIdx;

            /* Exclude points with absolute radial velocity greater than velocityInclusionThr */
            if (fabs(radialVelocity) > inParams->velocityInclusionThr)
            {
                continue;
            }
            if (inParams->forceVelocityToZero)
            {
                radialVelocity = 0;
            }

            range = (rangeIdx + rangeBinOffset) * cfarHwaObj->staticCfg.rangeStep;

            /* Subtract range bias */
            range -= cfarHwaObj->staticCfg.rangeBias;
            if (range < 0)
            {
                range = 0;
            }

            azimuthSgnBin = (float)azimuthSgnIdx + azimBinOffset + azimBinCorr;
            if (azimuthSgnBin > azimuthHalfFftSize)
            {
                azimuthSgnBin -= azimuthFullFftSize;
            }
            if (azimuthSgnBin < -azimuthHalfFftSize)
            {
                azimuthSgnBin += azimuthFullFftSize;
            }

            Wx = lambdaOverDistX * (azimuthSgnBin)*cfarHwaObj->oneOverAzimFftSize;
            x  = range * Wx;

            Wz = -lambdaOverDistZ * ((float)elevationSgnIdx + elevBinCorr) * cfarHwaObj->oneOverElevFftSize;

            z = range * Wz;

            temp = range * range - x * x - z * z;
            if (temp > 0)
            {
                y = sqrt(temp);
            }
            else
            {
                y = 0; // ToDo check this
            }

            detObjOut[numCfarDet].y        = y;
            detObjOut[numCfarDet].x        = x;
            detObjOut[numCfarDet].z        = z;
            detObjOut[numCfarDet].velocity = radialVelocity;
            detObjOut[numCfarDet].snr      = snrdB;
            detObjOut[numCfarDet].noise    = noisedB;

            // For Testing save point cloud indices (range,azimuth,elevation,doppler)
            if (cfarHwaObj->staticCfg.enableCfarPointCloudListWithIndices)
            {
                detObjIndOut[numCfarDet].rangeInd     = rangeIdx;
                detObjIndOut[numCfarDet].azimuthInd   = azimuthSgnIdx;
                detObjIndOut[numCfarDet].elevationInd = elevationSgnIdx;
                detObjIndOut[numCfarDet].dopplerInd   = dopplerSgnIdx;
            }

            numCfarDet++;
            if (numCfarDet >= cfarDetectionListSize)
            {
                break;
            }
        }
    }

    return numCfarDet;
}

uint32_t CFAR_DET_HWA_cfarRange_ProcList_1tx_1rx(CFARHwaObj                  *cfarHwaObj,
                                                 DPIF_DetMatrix              *detMatrix,
                                                 DPU_CFARProcHWA_InputParams *inParams,
                                                 DPU_CFARProcHWA_OutParams   *outParams)
{
    uint32_t  numCfarDet;
    uint32_t  numCfarRangeDet       = cfarHwaObj->numHwaCfarObjs;
    uint32_t *detMat                = detMatrix->data;
    uint16_t  numDopplerBins        = cfarHwaObj->staticCfg.numDopplerBins;
    uint32_t  cfarDetectionListSize = outParams->detObjOutMaxSize;

    DPU_CFARProcHWA_CfarDetOutput *cfarRangeDetOutList = cfarHwaObj->res.hwaMemOutDetList;
    // ToDo this is pointing to HWA memory
    // DPU_CFARProcHWA_HwaMaxOutput * maxAzimuthArray = (DPU_CFARProcHWA_HwaMaxOutput *) (((uint32_t) cfarRangeDetOutList) + CFAR_HWA_MEM_OFFSET_FOR_MAX_AZIMUTH_PER_RNG_BIN);
    // ToDo this is pointing to local memory

    uint8_t *dopplerIndexMatrix = cfarHwaObj->res.dopplerIndexMatrix.data;

    DPIF_PointCloudCartesianExt      *detObjOut    = outParams->detObjOut;
    DPIF_PointCloudRngAzimElevDopInd *detObjIndOut = outParams->detObjIndOut;

    uint32_t rangeIdx;
    uint32_t azimuthIdx;
    uint32_t i;
    uint32_t cfarCellNoise;
    int32_t  dopplerSgnIdx;
    int32_t  dopplerIdx;
    float    radialVelocity;
    float    snrdB, noisedB, range;
    float    x, y, z;

    int32_t selRangeMin = cfarHwaObj->detectionCfg.skipLeftRange;
    int32_t selRangeMax = cfarHwaObj->staticCfg.numRangeBins - cfarHwaObj->detectionCfg.skipRightRange - 1;

    float rangeBinOffset;


    // ToDo introduce dopplerFftSize and at the beginning define numDopplerbins as a function of clutter removal
    numCfarDet = 0;

    for (i = 0; i < numCfarRangeDet; i++)
    {
        rangeIdx      = cfarRangeDetOutList[i].cellIdx;
        azimuthIdx    = cfarRangeDetOutList[i].iterNum;
        cfarCellNoise = cfarRangeDetOutList[i].noise;

        if (azimuthIdx != 0)
        {
            continue;
        }
        /* Check if it is inside azimuth and range FOV */
        if ((rangeIdx >= selRangeMin) && (rangeIdx <= selRangeMax))
        {
            rangeBinOffset = 0.;

            if (rangeCompCfg.enabled)
            {
                // compute snrdB and noisedB earlier to avoid unnecessary calculations if point is to be removed
                noisedB = 20 * log10((float)cfarCellNoise);
                snrdB   = 20 * log10((float)detMat[rangeIdx]) - noisedB;
                if (snrdB < CFAR_DET_HWA_RangeComp_SNRNeededToKeepPoint((float)rangeIdx, 0))
                {
                    continue;
                }
            }

            /* Check for local peak in range domain */
            if (cfarHwaObj->cfarCfg.enableLocalMaxRange)
            {
                int32_t rangeLeftInd, rangeRightInd;
                rangeLeftInd  = rangeIdx > selRangeMin ? rangeIdx - 1 : rangeIdx;
                rangeRightInd = rangeIdx < selRangeMax ? rangeIdx + 1 : rangeIdx;
                if (!((detMat[rangeIdx] >= detMat[rangeLeftInd]) &&
                      (detMat[rangeIdx] >= detMat[rangeRightInd])))
                {
                    continue;
                }
            }


            /* Interpolate in range domain */
            if (cfarHwaObj->cfarCfg.enableInterpRangeDom)
            {
                if ((rangeIdx > selRangeMin) && (rangeIdx < selRangeMax))
                {
                    int32_t rangeLeftInd, rangeRightInd;
                    rangeLeftInd   = rangeIdx > selRangeMin ? rangeIdx - 1 : rangeIdx;
                    rangeRightInd  = rangeIdx < selRangeMax ? rangeIdx + 1 : rangeIdx;
                    float P        = 1.75; // Blackman window in range FFT
                    float xLeft    = detMat[rangeLeftInd];
                    float xRight   = detMat[rangeRightInd];
                    float x        = detMat[rangeIdx];
                    rangeBinOffset = P * (xRight - xLeft) / (x + xLeft + xRight);
                }
            }


            if ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2))
            {
                dopplerIdx = dopplerIndexMatrix[rangeIdx];
            }
            else
            {
                dopplerIdx = 0;
            }
            if (cfarHwaObj->staticCfg.isStaticClutterRemovalEnabled &&
                ((cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
                 (cfarHwaObj->staticCfg.selectCoherentPeakInDopplerDim == 2)))
            {
                dopplerIdx++;
            }


            if (cfarHwaObj->staticCfg.isDetMatrixLogScale)
            {
                noisedB = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)cfarCellNoise);
                snrdB   = DPU_CFAR_CONV_LOG2Q11_TO_20LOG10((float)detMat[rangeIdx]) - noisedB;
            }
            else
            {
                noisedB = 20 * log10f((float)cfarCellNoise);
                snrdB   = 20 * log10f((float)detMat[rangeIdx]) - noisedB;
            }

            dopplerSgnIdx = (int32_t)dopplerIdx;
            if (dopplerSgnIdx >= (int32_t)(numDopplerBins >> 1))
            {
                dopplerSgnIdx = dopplerSgnIdx - (int32_t)numDopplerBins;
            }
            radialVelocity = cfarHwaObj->staticCfg.dopplerStep * dopplerSgnIdx;

            /* Exclude points with absolute radial velocity greater than velocityInclusionThr */
            if (fabs(radialVelocity) > inParams->velocityInclusionThr)
            {
                continue;
            }
            if (inParams->forceVelocityToZero)
            {
                radialVelocity = 0;
            }

            range = (rangeIdx + rangeBinOffset) * cfarHwaObj->staticCfg.rangeStep;

            /* Subtract range bias */
            range -= cfarHwaObj->staticCfg.rangeBias;
            if (range < 0)
            {
                range = 0;
            }

            x = 0;
            z = 0;
            y = range;

            detObjOut[numCfarDet].y        = y;
            detObjOut[numCfarDet].x        = x;
            detObjOut[numCfarDet].z        = z;
            detObjOut[numCfarDet].velocity = radialVelocity;
            detObjOut[numCfarDet].snr      = snrdB;
            detObjOut[numCfarDet].noise    = noisedB;

            // For Testing save point cloud indices (range,azimuth,elevation,doppler)
            if (cfarHwaObj->staticCfg.enableCfarPointCloudListWithIndices)
            {
                detObjIndOut[numCfarDet].rangeInd     = rangeIdx;
                detObjIndOut[numCfarDet].azimuthInd   = 0;
                detObjIndOut[numCfarDet].elevationInd = 0;
                detObjIndOut[numCfarDet].dopplerInd   = dopplerSgnIdx;
            }

            numCfarDet++;
            if (numCfarDet >= cfarDetectionListSize)
            {
                break;
            }
        }
    }

    return numCfarDet;
}


DPU_CFARProcHWA_Handle DPU_CFARProcHWA_init(
    DPU_CFARProcHWA_InitParams *initCfg,
    int32_t                    *errCode)
{
    CFARHwaObj *cfarHwaObj = NULL;
    int32_t     status     = SystemP_SUCCESS;

    *errCode = 0;

    if ((initCfg == NULL) || (initCfg->hwaHandle == NULL))
    {
        *errCode = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    cfarHwaObj = (CFARHwaObj *)&gCfarProcHeapMem;
    if (cfarHwaObj == NULL)
    {
        *errCode = DPU_CFARPROCHWA_ENOMEM;
        goto exit;
    }

    /* Save for debugging */
#ifdef DBG_CFAR_HWA_OBJ_DPU
    gCfarHwaObj = cfarHwaObj;
#endif
    // RPMF printf("CFAR DPU: (CFARHwaObj *) 0x%08x\n", (uint32_t) cfarHwaObj);

    /* Initialize memory */
    memset((void *)cfarHwaObj, 0, sizeof(CFARHwaObj));

    /* Save init config params */
    cfarHwaObj->hwaHandle = initCfg->hwaHandle;

    /* Create DPU semaphores */
    status = SemaphoreP_constructBinary(&cfarHwaObj->edmaDoneSemaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_CFARPROCEDMA_ESEMA;
        goto exit;
    }

    status = SemaphoreP_constructBinary(&cfarHwaObj->hwaDone_semaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_CFARPROCHWA_ESEMA;
        goto exit;
    }


exit:

    if (*errCode < 0)
    {
        /* Free the object if it was allocated */
        if (cfarHwaObj != NULL)
        {
            cfarHwaObj = NULL;
        }
    }

    return ((DPU_CFARProcHWA_Handle)cfarHwaObj);
}

int32_t DPU_CFARProcHWA_config(
    DPU_CFARProcHWA_Handle  handle,
    DPU_CFARProcHWA_Config *cfarHwaCfg)
{
    int32_t  retVal = 0;
    uint32_t numAzimuthBins;
    uint32_t numCfarRunIterations;

    CFARHwaObj                   *cfarHwaObj = (CFARHwaObj *)handle;
    DPU_CFARProcHWA_HW_Resources *pRes       = &cfarHwaCfg->res;
    DPU_CFARProcHWA_StaticConfig *staticCfg  = &cfarHwaCfg->staticCfg;

    if (cfarHwaObj == NULL)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    if MEM_IS_NOT_ALIGN (pRes->hwaMemOutDetList,
                         DPU_CFARPROCHWA_HWA_MEM_OUT_RANGE_BYTE_ALIGNMENT)
    {
        retVal = DPU_CFARPROCHWA_ENOMEMALIGN_HWA_MEM_OUT_RANGE;
        goto exit;
    }

    if (staticCfg->angleDimension > 0)
    {
        /* 1D or 2D */
        numAzimuthBins = staticCfg->azimuthFftSize;
    }
    else
    {
        /* 0D  (1Tx-1Rx) */
        numAzimuthBins = 1;
    }
    /* Validate CFAR guard/noise len */
    if ((cfarHwaCfg->dynCfg.cfarCfg->guardLen + cfarHwaCfg->dynCfg.cfarCfg->winLen) * 2U >= staticCfg->numRangeBins)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    /* Validate CFAR numNoiseSamplesRight and  numNoiseSamplesLeft not equal to 1 */
    if (cfarHwaCfg->dynCfg.cfarCfg->winLen == 2)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    if (staticCfg->detectionHeatmapType == DPU_CFAR_RANGE_DOPPLER_HEATMAP)
    {
        numCfarRunIterations = staticCfg->numDopplerBins;
    }
    else if (staticCfg->detectionHeatmapType == DPU_CFAR_RANGE_AZIMUTH_HEATMAP)
    {
        numCfarRunIterations = numAzimuthBins;
    }
    else
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    if (staticCfg->detectionHeatmapType == DPU_CFAR_RANGE_AZIMUTH_HEATMAP)
    {
        if (staticCfg->isDetMatrixLogScale)
        {
            /* Not supported with range-azimut heatmap option*/
            retVal = DPU_CFARPROCHWA_EINVAL;
            goto exit;
        }
        if (cfarHwaCfg->dynCfg.cfarScndPassCfg->enabled)
        {
            /* Not supported with range-azimut heatmap option*/
            retVal = DPU_CFARPROCHWA_EINVAL;
            goto exit;
        }
    }

    /* Validate CFAR numNoiseSamplesRight and  numNoiseSamplesLeft not equal to 1 */
    if (cfarHwaCfg->dynCfg.cfarCfg->lookUpTableCorrectAzimuthDom == 1)
    {
        if (((cfarHwaCfg->staticCfg.azimuthFftSize * cfarHwaCfg->staticCfg.elevationFftSize) != sizeof(azimBinCorrArrayQ4)) ||
            ((cfarHwaCfg->staticCfg.azimuthFftSize * cfarHwaCfg->staticCfg.elevationFftSize) != sizeof(elevBinCorrArrayQ4)))
        {
            retVal = DPU_CFARPROCHWA_EINVAL;
            goto exit;
        }
    }

    /**************************************/
    /* Configure HWA                      */
    /**************************************/
    retVal = CFAR_DET_HWA_config_HWA(pRes,
                                     cfarHwaObj,
                                     pRes->hwaCfg.paramSetStartIdx,
                                     staticCfg->numRangeBins,
                                     numCfarRunIterations,
                                     cfarHwaCfg->dynCfg.cfarCfg,
                                     cfarHwaCfg->dynCfg.cfarScndPassCfg,
                                     pRes->hwaMemOutDetListSize,
                                     pRes->hwaCfg.dmaTrigSrcChan,
                                     HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemInp),
                                     HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemOutDetList),
                                     HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemOutRangeProfile),
                                     DPU_CFAR_RANGE_DOMAIN,
                                     staticCfg->isDetMatrixLogScale);
    if (retVal != 0)
    {
        goto exit;
    }

    /**************************************/
    /* Configure EDMA                     */
    /**************************************/
    retVal = CFAR_DET_HWA_config_EDMA(cfarHwaObj,
                                      cfarHwaObj->hwaHandle,
                                      pRes,
                                      staticCfg);
    if (retVal != 0)
    {
        goto exit;
    }

    CFAR_DET_HWA_saveConfiguration(cfarHwaObj, cfarHwaCfg);
    cfarHwaObj->oneOverAzimFftSize = 1. / cfarHwaCfg->staticCfg.azimuthFftSize;
    cfarHwaObj->oneOverElevFftSize = 1. / cfarHwaCfg->staticCfg.elevationFftSize;

exit:
    return retVal;
}

int32_t DPU_CFARProcHWA_GetNumUsedHwaParamSets(
    DPU_CFARProcHWA_Handle handle,
    uint8_t               *numUsedHwaParamSets)
{
    int32_t     retVal = 0;
    CFARHwaObj *cfarHwaObj;

    if (handle == NULL)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    cfarHwaObj = (CFARHwaObj *)handle;

    *numUsedHwaParamSets = (uint8_t)cfarHwaObj->res.hwaCfg.numParamSet;
exit:
    return retVal;
}


int32_t DPU_CFARProcHWA_process(
    DPU_CFARProcHWA_Handle       handle,
    DPIF_DetMatrix              *detMatrix,
    DPU_CFARProcHWA_InputParams *inputParams,
    DPU_CFARProcHWA_OutParams   *outParams)
{
    // volatile uint32_t   startTime;
    // volatile uint32_t   startTime1;
    int32_t retVal = 0;
    // uint32_t            waitTimeLocal = 0;

    CFARHwaObj *cfarHwaObj;

    if (handle == NULL)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    cfarHwaObj = (CFARHwaObj *)handle;

    // startTime = Cycleprofiler_getTimeStamp();

    /* Load HWA param sets */
    /*retVal = HWA_loadParams(cfarHwaObj->hwaHandle,
                            cfarHwaObj->res.hwaCfg.paramSetStartIdx,
                            cfarHwaObj->res.hwaCfg.numParamSet,
                            cfarHwaObj->hwaParamsSaveLoc.data);*/

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(cfarHwaObj->hwaHandle,
                                     CFARCAHWADoneIsrCallback,
                                     (void *)&cfarHwaObj->hwaDone_semaHandle);
    if (retVal != 0)
    {
        goto exit;
    }


    /* Prepare EDMA Detection matrix start address and output address for range profile */
    retVal = CFAR_DET_HWA_prepareEdmaRunTime(cfarHwaObj,
                                             detMatrix,
                                             outParams->rangeProfile);
    if (retVal != 0)
    {
        goto exit;
    }

    /* Prepare HWA Common Reg and CFAR threshold */
    retVal = CFAR_DET_HWA_prepareHwaRunTime(cfarHwaObj);
    if (retVal != 0)
    {
        goto exit;
    }

    retVal = CFAR_DET_HWA_triggerCFAR(cfarHwaObj);
    if (retVal != 0)
    {
        goto exit;
    }


    // startTime1 = Cycleprofiler_getTimeStamp();
    /* wait for the paramSets done interrupt */
    SemaphoreP_pend(&cfarHwaObj->hwaDone_semaHandle, SystemP_WAIT_FOREVER);
    /* wait for EDMA output */
    SemaphoreP_pend(&cfarHwaObj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);
    // waitTimeLocal += Cycleprofiler_getTimeStamp() - startTime1;

    HWA_readCFARPeakCountReg(cfarHwaObj->hwaHandle,
                             (uint8_t *)&cfarHwaObj->numHwaCfarObjs,
                             sizeof(uint16_t));
    if (cfarHwaObj->numHwaCfarObjs > cfarHwaObj->res.hwaMemOutDetListSize)
    {
        cfarHwaObj->numHwaCfarObjs = cfarHwaObj->res.hwaMemOutDetListSize;
    }

    if (cfarHwaObj->cfarScndPassCfg.enabled)
    {
        /* Prepare HWA Common Reg and CFAR threshold */
        retVal = CFAR_DET_HWA_prepareHwaRunTimeScndPass(cfarHwaObj);
        if (retVal != 0)
        {
            goto exit;
        }
        /* Enable the HWA */
        retVal = HWA_enable(cfarHwaObj->hwaHandle, 1);
        if (retVal != 0)
        {
            goto exit;
        }

        /* Trigger HWA to run the second pass CFAR */
        HWA_setSoftwareTrigger(cfarHwaObj->hwaHandle);

        /* wait for the paramSets done interrupt */
        SemaphoreP_pend(&cfarHwaObj->hwaDone_semaHandle, SystemP_WAIT_FOREVER);

        HWA_readCFARPeakCountReg(cfarHwaObj->hwaHandle,
                                 (uint8_t *)&cfarHwaObj->numHwaCfarScndPassObjs,
                                 sizeof(uint16_t));
        if (cfarHwaObj->numHwaCfarScndPassObjs > cfarHwaObj->res.hwaMemOutDetListSize)
        {
            cfarHwaObj->numHwaCfarScndPassObjs = cfarHwaObj->res.hwaMemOutDetListSize;
        }
    }

    if (cfarHwaObj->staticCfg.detectionHeatmapType == DPU_CFAR_RANGE_AZIMUTH_HEATMAP)
    {
        if (cfarHwaObj->staticCfg.angleDimension > 0)
        {
            /**** Process HWA CFAR detected points ****/
            cfarHwaObj->numHwaCfarObjs = CFAR_DET_HWA_cfarRange_ProcList(cfarHwaObj,
                                                                         detMatrix,
                                                                         inputParams,
                                                                         outParams);
        }
        else
        {
            /**** Process HWA CFAR detected points ****/
            cfarHwaObj->numHwaCfarObjs = CFAR_DET_HWA_cfarRange_ProcList_1tx_1rx(cfarHwaObj,
                                                                                 detMatrix,
                                                                                 inputParams,
                                                                                 outParams);
        }
    }
    else if (cfarHwaObj->staticCfg.detectionHeatmapType == DPU_CFAR_RANGE_DOPPLER_HEATMAP)
    {
        /**** Process HWA CFAR detected points ****/
        cfarHwaObj->numHwaCfarObjs = CFAR_DET_HWA_cfarRangeDoppler_ProcList(cfarHwaObj,
                                                                            detMatrix,
                                                                            inputParams,
                                                                            outParams);
    }
    cfarHwaObj->numProcess++;

    /* Disable the HWA */
    retVal = HWA_enable(cfarHwaObj->hwaHandle, 0);
    if (retVal != 0)
    {
        outParams->numCfarDetectedPoints = 0U;
        goto exit;
    }


    outParams->numCfarDetectedPoints = cfarHwaObj->numHwaCfarObjs;
    HWA_disableDoneInterrupt(cfarHwaObj->hwaHandle);
    // outParams->stats.waitTime = waitTimeLocal;
    // outParams->stats.processingTime = Cycleprofiler_getTimeStamp() - startTime - waitTimeLocal;
    outParams->stats.numProcess = cfarHwaObj->numProcess;
exit:
    return (retVal);
}

int32_t DPU_CFARProcHWA_control(
    DPU_CFARProcHWA_Handle handle,
    DPU_CFARProcHWA_Cmd    cmd,
    void                  *arg,
    uint32_t               argSize)
{
    int32_t                       retVal     = 0;
    CFARHwaObj                   *cfarHwaObj = (CFARHwaObj *)handle;
    DPU_CFARProcHWA_StaticConfig *staticCfg  = &cfarHwaObj->staticCfg;
    DPU_CFARProcHWA_HW_Resources *pRes       = &cfarHwaObj->res;

    /* Get rangeProc data object */
    if (cfarHwaObj == NULL)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

    switch (cmd)
    {
        case DPU_CFARProcHWA_Cmd_CfarCfg:
        {
            if ((argSize != sizeof(DPU_CFARProc_CfarCfg)) ||
                (arg == NULL))
            {
                retVal = DPU_CFARPROCHWA_EINVAL;
                goto exit;
            }
            else
            {
                /* Save configuration */
                memcpy((void *)&cfarHwaObj->cfarCfg, arg, argSize);

                /**************************************/
                /* CFAR RANGE DOMAIN                 */
                /**************************************/
                retVal = CFAR_DET_HWA_config_HWA(pRes,
                                                 cfarHwaObj->hwaHandle,
                                                 pRes->hwaCfg.paramSetStartIdx,
                                                 staticCfg->numRangeBins,
                                                 staticCfg->azimuthFftSize,
                                                 &cfarHwaObj->cfarCfg,
                                                 &cfarHwaObj->cfarScndPassCfg,
                                                 pRes->hwaMemOutDetListSize,
                                                 pRes->hwaCfg.dmaTrigSrcChan,
                                                 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemInp),
                                                 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemOutDetList),
                                                 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(pRes->hwaMemOutRangeProfile),
                                                 DPU_CFAR_RANGE_DOMAIN,
                                                 staticCfg->isDetMatrixLogScale);

                if (retVal != 0)
                {
                    goto exit;
                }
            }
        }
        break;
        case DPU_CFARProcHWA_Cmd_FovRangeCfg:
        {
            if ((argSize != sizeof(DPU_CFARProc_RangeFovCfg)) ||
                (arg == NULL))
            {
                retVal = DPU_CFARPROCHWA_EINVAL;
                goto exit;
            }
            else
            {
                /* Save configuration */
                CFAR_DET_HWA_ConfigRangeView(arg,
                                             cfarHwaObj->staticCfg.numRangeBins,
                                             cfarHwaObj->staticCfg.rangeStep,
                                             cfarHwaObj->staticCfg.rangeBias,
                                             &cfarHwaObj->detectionCfg);
            }
        }
        break;
        default:
            retVal = DPU_CFARPROCHWA_EINVAL;
            break;
    }
exit:
    return (retVal);
}

int32_t DPU_CFARProcHWA_deinit(DPU_CFARProcHWA_Handle handle)
{
    int32_t retVal = 0;
    if (handle == NULL)
    {
        retVal = DPU_CFARPROCHWA_EINVAL;
        goto exit;
    }

exit:
    return (retVal);
}
