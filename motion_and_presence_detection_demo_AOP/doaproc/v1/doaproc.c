/*
 * Copyright (C) 2024 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free,
 * non-exclusive license under copyrights and patents it now or hereafter
 * owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
 * this software subject to the terms herein.  With respect to the foregoing patent
 * license, such license is granted  solely to the extent that any such patent is necessary
 * to Utilize the software alone.  The patent license shall not apply to any combinations which
 * include this software, other than combinations with devices manufactured by or for TI ("TI Devices").
 * No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license (including the
 * above copyright notice and the disclaimer and (if applicable) source code license limitations below)
 * in the documentation and/or other materials provided with the distribution
 *
 * Redistribution and use in binary form, without modification, are permitted provided that the following
 * conditions are met:
 *
 *	* No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
 *     software provided in binary form.
 *	* any redistribution and use are licensed by TI for use only with TI Devices.
 *	* Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the source code are permitted
 * provided that the following conditions are met:
 *
 *   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
 *     TI for use only with TI Devices.
 *   * any redistribution and use of any object code compiled from the source code and any resulting derivative
 *     works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
 * promote products derived from this software without specific prior written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
#include <doaproc/v1/doaproc.h>
#include <doaproc/v1/doaprocinternal.h>

/* Flag to check input parameters */
#define DEBUG_CHECK_PARAMS 1

#define DPU_DOAPROC_BANK_0 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[0])
#define DPU_DOAPROC_BANK_1 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[1])
#define DPU_DOAPROC_BANK_2 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[2])
#define DPU_DOAPROC_BANK_3 HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[3])

/* HWA ping/pong buffers offset */
#define DPU_DOAPROC_SRC_PING_OFFSET HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[0])
#define DPU_DOAPROC_SRC_PONG_OFFSET HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[1])
#define DPU_DOAPROC_DST_PING_OFFSET HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[2])
#define DPU_DOAPROC_DST_PONG_OFFSET HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(obj->hwaMemBankAddr[3])

#define DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET DPU_DOAPROC_BANK_0

/**
 * @brief   Detection matrix data format ToDo should be defined in SDK
 */
#define DPU_DOAPROC_DPIF_DETMATRIX_FORMAT_2 2

/* User defined heap memory and handle */
#define DOAPROC_HEAP_MEM_SIZE (sizeof(DPU_DoaProc_Obj))

static uint8_t gDoaProcHeapMem[DOAPROC_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

int32_t doaProc_InternalLoop(
    DPU_DoaProc_Obj       *obj,
    DPU_DoaProc_OutParams *outParams

);

/*===========================================================
 *                    Internal Functions
 *===========================================================*/

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function.
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 */
static void doaProc_hwaDoneIsrCallback(void *arg)
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
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 */
static void doaProc_edmaDoneIsrCallback(Edma_IntrHandle intrHandle, void *arg)
{
    if (arg != NULL)
    {
        SemaphoreP_post((SemaphoreP_Object *)arg);
    }
}


/**
 *  @b Description
 *  @n
 *      function to implement zero-doppler difference.
 *
 *
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 */


static void replaceZeroDoppWithDiff(uint32_t paramSet, void *arg)
{
    // Cast type back to DPIF_DetMatrix for decoding
    DPIF_DetMatrix *zeroDopplerMatrix = &((DPU_DoaProc_HW_Resources *)arg)->zeroDopplerPrevFrameMatrix;

    // Cast type back to DPIF_DetMatrix for decoding
    uint32_t *diffFramesPopulated = &((DPU_DoaProc_HW_Resources *)arg)->diffFramesPopulated;

    // Data inside matrix is a 1D array of cmplx32ImRe_t
    cmplx32ImRe_t *zeroDopplerMatrixPrevFrameData = (cmplx32ImRe_t *)(zeroDopplerMatrix->data);

    cmplx32ImRe_t tempDopplerBinCurrFrame, diffDoppler;
    int32_t       channelIdx, result;
    for (channelIdx = 0; channelIdx < zDopDiffFramesObj.numChannels; channelIdx++)
    {
        // Access HWA M2, where the doppler FFT data from the current frame is stored
        tempDopplerBinCurrFrame = zDopDiffFramesObj.HWAMemBank2Address[channelIdx];

        // Compute the difference between the current doppler FFT and the previous
        diffDoppler.real = tempDopplerBinCurrFrame.real - zeroDopplerMatrixPrevFrameData[zDopDiffFramesObj.rangeBinIdx * zDopDiffFramesObj.numChannels + channelIdx].real;
        diffDoppler.imag = tempDopplerBinCurrFrame.imag - zeroDopplerMatrixPrevFrameData[zDopDiffFramesObj.rangeBinIdx * zDopDiffFramesObj.numChannels + channelIdx].imag;

        if (*diffFramesPopulated == 1)
        {
            // Populate the HWA Bank 2 with the difference of the 0-doppler frames
            zDopDiffFramesObj.HWAMemBank2Address[channelIdx] = diffDoppler;
        }
        /* In the first frame, we cannot compute a difference frame because there is no previous frame,
          in this case, set the 0-doppler bin to zero */
        else
        {
            zDopDiffFramesObj.HWAMemBank2Address[channelIdx].real = 0;
            zDopDiffFramesObj.HWAMemBank2Address[channelIdx].imag = 0;
        }

        // Put the current 0-doppler estimate in the buffer to compute the difference on the next frame
        zeroDopplerMatrixPrevFrameData[zDopDiffFramesObj.rangeBinIdx * zDopDiffFramesObj.numChannels + channelIdx] = tempDopplerBinCurrFrame;
    }
    // Increment the placeholder in the zeroDopplerMatrixData so we access the correct range bin next time
    zDopDiffFramesObj.rangeBinIdx++;

    // Restart when all range bins have been processed
    if (zDopDiffFramesObj.rangeBinIdx == zDopDiffFramesObj.numRangeBins)
    {
        zDopDiffFramesObj.rangeBinIdx = 0;
    }
    // Trigger the HWA.
    result = HWA_setSoftwareTrigger(zDopDiffFramesObj.HwaHandle);
    if (result != 0)
    {
        while (1)
            ;
    }
}


#if 1
/**
 *  @b Description
 *  @n
 *      Configures HWA for Doppler processing.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 *
 *  @retval error code.
 */
static inline int32_t doaProc_configHwa_v2(
    DPU_DoaProc_Obj    *obj,
    DPU_DoaProc_Config *cfg)
{
    HWA_ParamConfig     hwaParamCfg;
    HWA_InterruptConfig paramISRConfig;
    uint32_t            paramsetIdx = 0;
    int32_t             retVal      = 0U;

    DPU_DoaProc_HWA_Option_Cfg *rngGateCfg;
    uint32_t                    idx;
    uint32_t                    numAntRow, numAntCol;
    uint32_t                    rowIdx;

    uint16_t numOutputDopplerBins;
    uint16_t skipOutputDopplerBins;
    uint16_t numAzimuthBins;

    uint8_t destChan;

    if (cfg->staticCfg.isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins  = cfg->staticCfg.numDopplerBins - 1;
        skipOutputDopplerBins = 1;
    }
    else
    {
        numOutputDopplerBins  = cfg->staticCfg.numDopplerBins;
        skipOutputDopplerBins = 0;
    }


#if 0 // ToDo
    /* Check if we have the correct number of paramsets.*/
    if(cfg->hwRes.hwaCfg.numParamSets != (2 * cfg->staticCfg.numTxAntennas + 2))
    {
        retVal = DPU_DOAPROC_EHWARES;
        goto exit;
    }
#endif

    rngGateCfg = &cfg->hwRes.hwaCfg.doaRngGateCfg;
    numAntRow  = cfg->staticCfg.numAntRow;
    numAntCol  = cfg->staticCfg.numAntCol;


    /* Currently no scaling in Doppler FFT. */
    obj->dopFftSumDiv = 0; // mathUtils_ceilLog2(cfg->staticCfg.numDopplerBins);


    paramsetIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /********************************************************************************/
        /*                    Rx channel phase compensation                             */
        /********************************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
        hwaParamCfg.dmaTriggerSrc = cfg->hwRes.hwaCfg.dmaTrigSrcChan;

        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(cfg->hwRes.hwaCfg.hwaMemInpAddr); // DPU_DOAPROC_BANK_3;
        hwaParamCfg.source.srcAcnt = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) - 1;

        hwaParamCfg.source.srcAIdx          = sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = cfg->staticCfg.numDopplerChirps - 1;
        hwaParamCfg.source.srcBIdx          = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0;
        hwaParamCfg.source.srcScale         = 8;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_DOAPROC_BANK_0;
        hwaParamCfg.dest.dstAcnt        = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) - 1; // this is samples - 1
        hwaParamCfg.dest.dstAIdx        = sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstBIdx        = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate   = 0;
        hwaParamCfg.dest.dstScale       = 0;

        hwaParamCfg.accelModeArgs.fftMode.fftEn      = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn   = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

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
    }
    /********************************************************************************/
    /*                    Doppler FFT Configuration                                 */
    /********************************************************************************/
    for (idx = 0; idx < rngGateCfg->numDopFftParams; idx++)
    {
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        if ((idx == 0) && (!cfg->staticCfg.isRxChGainPhaseCompensationEnabled))
        {
            hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
            hwaParamCfg.dmaTriggerSrc = cfg->hwRes.hwaCfg.dmaTrigSrcChan;
        }
        else
        {
            hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;
        }
        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = DPU_DOAPROC_BANK_0 + rngGateCfg->dopFftCfg[idx].srcAddrOffset * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numDopplerChirps - 1; // size in samples - 1

        hwaParamCfg.source.srcAIdx          = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = rngGateCfg->dopFftCfg[idx].srcBcnt - 1;
        hwaParamCfg.source.srcBIdx          = sizeof(cmplx16ImRe_t) * rngGateCfg->dopFftCfg[idx].srcBidx;
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0; // no conjugate
        hwaParamCfg.source.srcScale         = 8;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_DOAPROC_BANK_2 + rngGateCfg->dopFftCfg[idx].dstAddrOffset * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstAcnt        = (cfg->staticCfg.numDopplerBins) - 1; // this is samples - 1
        hwaParamCfg.dest.dstAIdx        = numAntRow * numAntCol * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstBIdx        = sizeof(cmplx32ImRe_t) * rngGateCfg->dopFftCfg[idx].dstBidx;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;
        hwaParamCfg.dest.dstSkipInit    = skipOutputDopplerBins; // Doppler zero skipped - CLUTTER RMOVAL

        hwaParamCfg.accelModeArgs.fftMode.fftEn   = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = cfg->staticCfg.log2NumDopplerBins;

        /* scaling is disabled in all stages */
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = (cfg->staticCfg.numDopplerBins - 1) >> 5;


        hwaParamCfg.accelModeArgs.fftMode.interfZeroOutEn    = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowEn           = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowStart        = 0;
        hwaParamCfg.accelModeArgs.fftMode.winSymm            = 0;
        hwaParamCfg.accelModeArgs.fftMode.winInterpolateMode = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn           = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode         = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if (rngGateCfg->dopFftCfg[idx].scale == 0)
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_SCALAR_MULT;
        }
        else
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        }
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

        if ((idx == rngGateCfg->numDopFftParams - 1) && (cfg->staticCfg.isDiffAcrossFramesEnabled))
        {
            zDopDiffFramesObj.rangeBinIdx        = 0;
            zDopDiffFramesObj.numRangeBins       = cfg->staticCfg.numRangeBins;
            zDopDiffFramesObj.numChannels        = numAntRow * numAntCol;
            zDopDiffFramesObj.HWAMemBank2Address = (cmplx32ImRe_t *)obj->hwaMemBankAddr[2];
            zDopDiffFramesObj.HwaHandle          = obj->hwaHandle;
            /* trigger the CPU to overwrite the zero-doppler with across-frame difference is disabled for this paramset*/
            paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_CPU;
            paramISRConfig.dma.dstChannel    = 0;
            paramISRConfig.cpu.callbackFn    = replaceZeroDoppWithDiff;
            paramISRConfig.cpu.callbackArg   = (void *)(&(cfg->hwRes));
            retVal                           = HWA_enableParamSetInterrupt(obj->hwaHandle, paramsetIdx, &paramISRConfig);
            if (retVal != 0)
            {
                goto exit;
            }
        }

        paramsetIdx++;
    }

    /***************************************************************/
    /******************* Configure Azimuth FFT *********************/
    /***************************************************************/
    if (cfg->staticCfg.angleDimension > 0)
    {
        numAzimuthBins = cfg->staticCfg.azimuthFftSize;
    }
    else
    {
        numAzimuthBins = 1;
    }

    for (rowIdx = 0; rowIdx < numAntRow; rowIdx++)
    {
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        if ((rowIdx == 0) && (cfg->staticCfg.isDiffAcrossFramesEnabled))
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_SOFTWARE;
        }
        else
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
        }

        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAddr        = (uint16_t)(DPU_DOAPROC_BANK_2 + rowIdx * numAntCol * sizeof(cmplx32ImRe_t));
        hwaParamCfg.source.srcAcnt        = numAntCol - 1;
        hwaParamCfg.source.srcAIdx        = sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBIdx        = numAntCol * numAntRow * sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBcnt        = (numOutputDopplerBins)-1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate   = 0; // no conjugate
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        if (numAzimuthBins > 1)
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = HWA_FEATURE_BIT_ENABLE;
        }
        else
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = HWA_FEATURE_BIT_DISABLE;
        }
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = mathUtils_ceilLog2(numAzimuthBins); // assumes power of 2;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 1; // Azimuth and elavation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
        hwaParamCfg.accelModeArgs.fftMode.windowStart      = cfg->hwRes.hwaCfg.winRamOffset;
        hwaParamCfg.accelModeArgs.fftMode.winSymm          = cfg->hwRes.hwaCfg.winSym;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; // ToDo Tweak this

        hwaParamCfg.dest.dstAddr                     = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                              rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(cmplx32ImRe_t));
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if (cfg->staticCfg.angleDimension == 2)
        {
            /*With Elevation*/
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.dest.dstAddr                   = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                                  rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(cmplx32ImRe_t));
            hwaParamCfg.dest.dstAcnt                   = numAzimuthBins - 1;
            hwaParamCfg.dest.dstAIdx                   = sizeof(cmplx32ImRe_t);
            hwaParamCfg.dest.dstBIdx                   = numAzimuthBins * sizeof(cmplx32ImRe_t);
            hwaParamCfg.dest.dstSign                   = HWA_SAMPLES_SIGNED;
            hwaParamCfg.dest.dstRealComplex            = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth                  = HWA_SAMPLES_WIDTH_32BIT;
        }
        else
        {
            /*No Elevation*/
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
            hwaParamCfg.dest.dstAddr                   = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                                  rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(uint32_t));
            hwaParamCfg.dest.dstAcnt                   = numAzimuthBins - 1;
            hwaParamCfg.dest.dstAIdx                   = sizeof(uint32_t);
            hwaParamCfg.dest.dstBIdx                   = numAzimuthBins * sizeof(uint32_t);
            hwaParamCfg.dest.dstSign                   = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstRealComplex            = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth                  = HWA_SAMPLES_WIDTH_32BIT;
        }

        hwaParamCfg.dest.dstConjugate = 0; // no conjugate
        hwaParamCfg.dest.dstScale     = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        paramsetIdx++;
    }


    if (cfg->staticCfg.angleDimension == 2)
    {
        /* [Elevation FFT + Max]  [Doppler Max/Sum]  */
        /*****************************************************************/
        /******************* Configure Elevation FFT *********************/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));

        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAddr        = (uint16_t)DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET; // ToDo for debugging we use all 4 banks
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcAcnt        = numAntRow - 1;
        hwaParamCfg.source.srcBcnt        = (numAzimuthBins * numOutputDopplerBins) - 1;
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * numOutputDopplerBins * sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBIdx        = sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate   = 0; // no conjugate
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = mathUtils_ceilLog2(cfg->staticCfg.elevationFftSize); // assumes power of 2;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 1; // Azimuth and elavation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
        hwaParamCfg.accelModeArgs.fftMode.windowStart      = cfg->hwRes.hwaCfg.winRamOffset;
        hwaParamCfg.accelModeArgs.fftMode.winSymm          = cfg->hwRes.hwaCfg.winSym;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; // ToDo Tweak this
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_MAX_STATS;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn         = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
        hwaParamCfg.dest.dstAddr                           = (uint16_t)DPU_DOAPROC_BANK_2;
        hwaParamCfg.dest.dstSign                           = HWA_SAMPLES_UNSIGNED;

        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;
    }

    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
        (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        /*****************************************************************/
        /******** Configure MAXIMUM along Doppler dimension      *********/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_2) + sizeof(uint32_t);
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_0);
        }
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * sizeof(DPU_DoaProc_HwaMaxOutput);
        hwaParamCfg.source.srcBIdx        = sizeof(DPU_DoaProc_HwaMaxOutput);
        hwaParamCfg.source.srcBcnt        = numAzimuthBins - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate   = 0;
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_MAX_STATS; // coherent integration (MAX)

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_1 + 0x2000; // ToDo +0x2000 is temporary
        }
        else
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_2 + 0x2000; // ToDo +0x2000 is temporary
        }
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        if ((cfg->staticCfg.angleDimension == 1) &&
            (cfg->staticCfg.selectCoherentPeakInDopplerDim == 1))
        {
            retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                         cfg->hwRes.edmaCfg.edmaDetMatOut.channel, // ToDo Add the correct output channel
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

            obj->hwaParamStopIdx = paramsetIdx;
        }
        else
        {
            paramsetIdx++;
        }
    }

    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 0) ||
        (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        /*****************************************************************/
        /******** Configure SUM along Doppler dimension          *********/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_2) + sizeof(uint32_t);
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_0);
        }
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * sizeof(DPU_DoaProc_HwaMaxOutput);
        hwaParamCfg.source.srcBIdx        = sizeof(DPU_DoaProc_HwaMaxOutput);
        hwaParamCfg.source.srcBcnt        = numAzimuthBins - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate   = 0;
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_SUM_STATS; // non-coherent integration (SUM)

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_1 + 0x0000; // ToDo +0x2000 is temporary
        }
        else
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_2 + 0x0000; // ToDo +0x2000 is temporary
        }
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }


        retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                     cfg->hwRes.edmaCfg.edmaDetMatOut.channel, // ToDo Add the correct output channel
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

        obj->hwaParamStopIdx = paramsetIdx;
    }


exit:
    return (retVal);
}
#endif


/*
 *  @b Description
 *  @n *      Configures HWA for Doppler processing.
 *
 *  @param[in] obj    - DPU obj
 *  @param[in] cfg    - DPU configuration
 *
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 *
 *  @retval error code.
 */
static inline int32_t doaProc_configHwa(
    DPU_DoaProc_Obj    *obj,
    DPU_DoaProc_Config *cfg)
{
    HWA_ParamConfig     hwaParamCfg;
    HWA_InterruptConfig paramISRConfig;
    uint32_t            paramsetIdx = 0;
    int32_t             retVal      = 0U;

    DPU_DoaProc_HWA_Option_Cfg *rngGateCfg;
    uint32_t                    idx;
    uint32_t                    numAntRow, numAntCol;
    uint32_t                    rowIdx;

    uint16_t numOutputDopplerBins;
    uint16_t skipOutputDopplerBins;
    uint16_t numElevationBins;
    uint16_t numAzimuthBins;

    uint8_t destChan;

    if (cfg->staticCfg.isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins  = cfg->staticCfg.numDopplerBins - 1;
        skipOutputDopplerBins = 1;
    }
    else
    {
        numOutputDopplerBins  = cfg->staticCfg.numDopplerBins;
        skipOutputDopplerBins = 0;
    }


#if 0 // ToDo
    /* Check if we have the correct number of paramsets.*/
    if(cfg->hwRes.hwaCfg.numParamSets != (2 * cfg->staticCfg.numTxAntennas + 2))
    {
        retVal = DPU_DOAPROC_EHWARES;
        goto exit;
    }
#endif

    rngGateCfg = &cfg->hwRes.hwaCfg.doaRngGateCfg;
    numAntRow  = cfg->staticCfg.numAntRow;
    numAntCol  = cfg->staticCfg.numAntCol;


    /* Currently no scaling in Doppler FFT. */
    obj->dopFftSumDiv = 0; // mathUtils_ceilLog2(cfg->staticCfg.numDopplerBins);


    paramsetIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /********************************************************************************/
        /*                    Rx channel phase compensation                             */
        /********************************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
        hwaParamCfg.dmaTriggerSrc = cfg->hwRes.hwaCfg.dmaTrigSrcChan;

        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = HWADRV_ADDR_TRANSLATE_CPU_TO_HWA(cfg->hwRes.hwaCfg.hwaMemInpAddr); // DPU_DOAPROC_BANK_3;
        hwaParamCfg.source.srcAcnt = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) - 1;

        hwaParamCfg.source.srcAIdx          = sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = cfg->staticCfg.numDopplerChirps - 1;
        hwaParamCfg.source.srcBIdx          = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0;
        hwaParamCfg.source.srcScale         = 8;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_DOAPROC_BANK_0;
        hwaParamCfg.dest.dstAcnt        = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) - 1; // this is samples - 1
        hwaParamCfg.dest.dstAIdx        = sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstBIdx        = (cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas) * sizeof(cmplx16ImRe_t);
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate   = 0;
        hwaParamCfg.dest.dstScale       = 0;

        hwaParamCfg.accelModeArgs.fftMode.fftEn      = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn   = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

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
    }
    /********************************************************************************/
    /*                    Doppler FFT Configuration                                 */
    /********************************************************************************/
    for (idx = 0; idx < rngGateCfg->numDopFftParams; idx++)
    {
        memset((void *)&hwaParamCfg, 0, sizeof(HWA_ParamConfig));
        if ((idx == 0) && (!cfg->staticCfg.isRxChGainPhaseCompensationEnabled))
        {
            hwaParamCfg.triggerMode   = HWA_TRIG_MODE_DMA;
            hwaParamCfg.dmaTriggerSrc = cfg->hwRes.hwaCfg.dmaTrigSrcChan;
        }
        else
        {
            hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
            hwaParamCfg.dmaTriggerSrc = 0;
        }
        hwaParamCfg.accelMode      = HWA_ACCELMODE_FFT;
        hwaParamCfg.source.srcAddr = DPU_DOAPROC_BANK_0 + rngGateCfg->dopFftCfg[idx].srcAddrOffset * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcAcnt = cfg->staticCfg.numDopplerChirps - 1; // size in samples - 1

        hwaParamCfg.source.srcAIdx          = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * sizeof(cmplx16ImRe_t);
        hwaParamCfg.source.srcBcnt          = rngGateCfg->dopFftCfg[idx].srcBcnt - 1;
        hwaParamCfg.source.srcBIdx          = sizeof(cmplx16ImRe_t) * rngGateCfg->dopFftCfg[idx].srcBidx;
        hwaParamCfg.source.srcShift         = 0;
        hwaParamCfg.source.srcCircShiftWrap = 0;
        hwaParamCfg.source.srcRealComplex   = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth         = HWA_SAMPLES_WIDTH_16BIT;
        hwaParamCfg.source.srcSign          = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate     = 0; // no conjugate
        hwaParamCfg.source.srcScale         = 8;
        hwaParamCfg.source.bpmEnable        = 0;
        hwaParamCfg.source.bpmPhase         = 0;

        hwaParamCfg.dest.dstAddr        = DPU_DOAPROC_BANK_2 + rngGateCfg->dopFftCfg[idx].dstAddrOffset * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstAcnt        = (cfg->staticCfg.numDopplerBins) - 1; // this is samples - 1
        hwaParamCfg.dest.dstAIdx        = numAntRow * numAntCol * sizeof(cmplx32ImRe_t);
        hwaParamCfg.dest.dstBIdx        = sizeof(cmplx32ImRe_t) * rngGateCfg->dopFftCfg[idx].dstBidx;
        hwaParamCfg.dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.dest.dstWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;
        hwaParamCfg.dest.dstSkipInit    = skipOutputDopplerBins; // Doppler zero skipped - CLUTTER RMOVAL

        hwaParamCfg.accelModeArgs.fftMode.fftEn   = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize = cfg->staticCfg.log2NumDopplerBins;

        /* scaling is disabled in all stages */
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = (cfg->staticCfg.numDopplerBins - 1) >> 5;


        hwaParamCfg.accelModeArgs.fftMode.interfZeroOutEn    = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowEn           = 0; // disabled
        hwaParamCfg.accelModeArgs.fftMode.windowStart        = 0;
        hwaParamCfg.accelModeArgs.fftMode.winSymm            = 0;
        hwaParamCfg.accelModeArgs.fftMode.winInterpolateMode = 0;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn           = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode         = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if (rngGateCfg->dopFftCfg[idx].scale == 0)
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_SCALAR_MULT;
        }
        else
        {
            hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        }
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

        if ((idx == rngGateCfg->numDopFftParams - 1) && (cfg->staticCfg.isDiffAcrossFramesEnabled))
        {
            zDopDiffFramesObj.numRangeBins       = cfg->staticCfg.numRangeBins;
            zDopDiffFramesObj.numChannels        = numAntRow * numAntCol;
            zDopDiffFramesObj.HWAMemBank2Address = (cmplx32ImRe_t *)obj->hwaMemBankAddr[2];
            zDopDiffFramesObj.HwaHandle          = obj->hwaHandle;
            /* trigger the CPU to overwrite the zero-doppler with across-frame difference is disabled for this paramset*/
            paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_CPU;
            paramISRConfig.dma.dstChannel    = 0;
            paramISRConfig.cpu.callbackFn    = replaceZeroDoppWithDiff;
            paramISRConfig.cpu.callbackArg   = (void *)(&(cfg->hwRes));
            retVal                           = HWA_enableParamSetInterrupt(obj->hwaHandle, paramsetIdx, &paramISRConfig);
            if (retVal != 0)
            {
                goto exit;
            }
        }

        paramsetIdx++;
    }

    /***************************************************************/
    /******************* Configure Azimuth FFT *********************/
    /***************************************************************/
    if (cfg->staticCfg.angleDimension > 0)
    {
        numAzimuthBins = cfg->staticCfg.azimuthFftSize;
    }
    else
    {
        numAzimuthBins = 1;
    }

    for (rowIdx = 0; rowIdx < numAntRow; rowIdx++)
    {
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        if ((rowIdx == 0) && (cfg->staticCfg.isDiffAcrossFramesEnabled))
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_SOFTWARE;
        }
        else
        {
            hwaParamCfg.triggerMode = HWA_TRIG_MODE_IMMEDIATE;
        }

        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAddr        = (uint16_t)(DPU_DOAPROC_BANK_2 + rowIdx * numAntCol * sizeof(cmplx32ImRe_t));
        hwaParamCfg.source.srcAcnt        = numAntCol - 1;
        hwaParamCfg.source.srcAIdx        = sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBIdx        = numAntCol * numAntRow * sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBcnt        = (numOutputDopplerBins)-1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate   = 0; // no conjugate
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        if (numAzimuthBins > 1)
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = HWA_FEATURE_BIT_ENABLE;
        }
        else
        {
            hwaParamCfg.accelModeArgs.fftMode.fftEn = HWA_FEATURE_BIT_DISABLE;
        }
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = mathUtils_ceilLog2(numAzimuthBins); // assumes power of 2;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 1; // Azimuth and elavation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
        hwaParamCfg.accelModeArgs.fftMode.windowStart      = cfg->hwRes.hwaCfg.winRamOffset;
        hwaParamCfg.accelModeArgs.fftMode.winSymm          = cfg->hwRes.hwaCfg.winSym;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; // ToDo Tweak this

        hwaParamCfg.dest.dstAddr                     = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                              rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(cmplx32ImRe_t));
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT;

        if (cfg->staticCfg.angleDimension == 2)
        {
            /*With Elevation*/
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
            hwaParamCfg.dest.dstAddr                   = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                                  rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(cmplx32ImRe_t));
            hwaParamCfg.dest.dstAcnt                   = numAzimuthBins - 1;
            hwaParamCfg.dest.dstAIdx                   = sizeof(cmplx32ImRe_t);
            hwaParamCfg.dest.dstBIdx                   = numAzimuthBins * sizeof(cmplx32ImRe_t);
            hwaParamCfg.dest.dstSign                   = HWA_SAMPLES_SIGNED;
            hwaParamCfg.dest.dstRealComplex            = HWA_SAMPLES_FORMAT_COMPLEX;
            hwaParamCfg.dest.dstWidth                  = HWA_SAMPLES_WIDTH_32BIT;
        }
        else
        {
            /*No Elevation*/
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
            hwaParamCfg.dest.dstAddr                   = (uint16_t)(DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET +
                                                  rowIdx * (numOutputDopplerBins)*numAzimuthBins * sizeof(uint32_t));
            hwaParamCfg.dest.dstAcnt                   = numAzimuthBins - 1;
            hwaParamCfg.dest.dstAIdx                   = sizeof(uint32_t);
            hwaParamCfg.dest.dstBIdx                   = numAzimuthBins * sizeof(uint32_t);
            hwaParamCfg.dest.dstSign                   = HWA_SAMPLES_UNSIGNED;
            hwaParamCfg.dest.dstRealComplex            = HWA_SAMPLES_FORMAT_REAL;
            hwaParamCfg.dest.dstWidth                  = HWA_SAMPLES_WIDTH_32BIT;
        }

        hwaParamCfg.dest.dstConjugate = 0; // no conjugate
        hwaParamCfg.dest.dstScale     = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        paramsetIdx++;
    }


    if (cfg->staticCfg.angleDimension == 2)
    {
        /* [Elevation FFT]  [Doppler Max/Sum] [Elevation Max] */
        /*****************************************************************/
        /******************* Configure Elevation FFT *********************/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));

        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAddr        = (uint16_t)DPU_DOAPROC_AZIMUTH_OUTPUT_BASE_OFFSET; // ToDo for debugging we use all 4 banks
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcAcnt        = numAntRow - 1;
        hwaParamCfg.source.srcBcnt        = (numAzimuthBins * numOutputDopplerBins) - 1;
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * numOutputDopplerBins * sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcBIdx        = sizeof(cmplx32ImRe_t);
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_SIGNED;
        hwaParamCfg.source.srcConjugate   = 0; // no conjugate
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 1;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = mathUtils_ceilLog2(cfg->staticCfg.elevationFftSize); // assumes power of 2;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 1; // Azimuth and elavation share FFT window. Window is 1,-1,1,-1... to achieve "fffshift" in spectral domain
        hwaParamCfg.accelModeArgs.fftMode.windowStart      = cfg->hwRes.hwaCfg.winRamOffset;
        hwaParamCfg.accelModeArgs.fftMode.winSymm          = cfg->hwRes.hwaCfg.winSym;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; // ToDo Tweak this
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_DEFAULT;
        hwaParamCfg.accelModeArgs.fftMode.magLogEn         = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED;
        hwaParamCfg.dest.dstAddr                           = (uint16_t)DPU_DOAPROC_BANK_2;
        hwaParamCfg.dest.dstAcnt                           = cfg->staticCfg.elevationFftSize - 1;
        hwaParamCfg.dest.dstBIdx                           = sizeof(uint32_t) * cfg->staticCfg.elevationFftSize;
        hwaParamCfg.dest.dstAIdx                           = sizeof(uint32_t);
        hwaParamCfg.dest.dstRealComplex                    = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.dest.dstWidth                          = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.dest.dstConjugate                      = 0; // no conjugate
        hwaParamCfg.dest.dstScale                          = 8;
        hwaParamCfg.dest.dstSign                           = HWA_SAMPLES_UNSIGNED;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }
        paramsetIdx++;
    }

    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
        (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        /*****************************************************************/
        /******** Configure MAXIMUM along Doppler dimension      *********/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_2);
            numElevationBins           = cfg->staticCfg.elevationFftSize;
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_0);
            numElevationBins           = 1;
        }
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * numElevationBins * sizeof(uint32_t);
        hwaParamCfg.source.srcBIdx        = sizeof(uint32_t);
        hwaParamCfg.source.srcBcnt        = (numAzimuthBins * numElevationBins) - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate   = 0;
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_MAX_STATS; // coherent integration (MAX)

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_1 + 0x2000; // ToDo +0x2000 is temporary
        }
        else
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_2 + 0x2000; // ToDo +0x2000 is temporary
        }
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        if ((cfg->staticCfg.angleDimension == 1) &&
            (cfg->staticCfg.selectCoherentPeakInDopplerDim == 1))
        {
            retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                         cfg->hwRes.edmaCfg.edmaDetMatOut.channel, // ToDo Add the correct output channel
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

            obj->hwaParamStopIdx = paramsetIdx;
        }
        else
        {
            paramsetIdx++;
        }
    }

    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 0) ||
        (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        /*****************************************************************/
        /******** Configure SUM along Doppler dimension          *********/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        hwaParamCfg.source.srcAcnt = numOutputDopplerBins - 1;

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_2);
            numElevationBins           = cfg->staticCfg.elevationFftSize;
        }
        else
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_0);
            numElevationBins           = 1;
        }
        hwaParamCfg.source.srcAIdx        = numAzimuthBins * numElevationBins * sizeof(uint32_t);
        hwaParamCfg.source.srcBIdx        = sizeof(uint32_t);
        hwaParamCfg.source.srcBcnt        = (numAzimuthBins * numElevationBins) - 1;
        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate   = 0;
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_SUM_STATS; // non-coherent integration (SUM)

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_1 + 0x0000; // ToDo +0x2000 is temporary
        }
        else
        {
            hwaParamCfg.dest.dstAddr = (uint16_t)DPU_DOAPROC_BANK_2 + 0x0000; // ToDo +0x2000 is temporary
        }
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        if (cfg->staticCfg.angleDimension <= 1)
        {
            /* 0D or 1D */
            retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                         cfg->hwRes.edmaCfg.edmaDetMatOut.channel, // ToDo Add the correct output channel
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

            obj->hwaParamStopIdx = paramsetIdx;
        }
        else
        {
            paramsetIdx++;
        }
    }

    if (cfg->staticCfg.angleDimension == 2)
    {
        /*****************************************************************/
        /************ Configure Maximum along Elevation dimension *******/
        /*****************************************************************/
        memset((void *)&hwaParamCfg, 0, sizeof(hwaParamCfg));
        hwaParamCfg.triggerMode   = HWA_TRIG_MODE_IMMEDIATE;
        hwaParamCfg.dmaTriggerSrc = 0;

        if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 1)
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_1 + sizeof(uint32_t) + 0x2000); // Skip 4Bytes index field, position to peak field.
        }
        else if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 0) ||
                 (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
        {
            hwaParamCfg.source.srcAddr = (uint16_t)(DPU_DOAPROC_BANK_1 + sizeof(uint32_t) + 0x0000); // Skip 4Bytes index field, position to peak field.
        }
        else
        {
            retVal = DPU_DOAPROC_EINVAL;
            goto exit;
        }

        if (cfg->staticCfg.angleDimension == 2)
        {
            hwaParamCfg.source.srcAcnt = cfg->staticCfg.elevationFftSize - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_DoaProc_HwaMaxOutput);
            hwaParamCfg.source.srcBIdx = cfg->staticCfg.elevationFftSize * sizeof(DPU_DoaProc_HwaMaxOutput);
            hwaParamCfg.source.srcBcnt = numAzimuthBins - 1;
        }
        else
        {
            hwaParamCfg.source.srcAcnt = 1 - 1;
            hwaParamCfg.source.srcAIdx = sizeof(DPU_DoaProc_HwaMaxOutput);
            hwaParamCfg.source.srcBIdx = sizeof(DPU_DoaProc_HwaMaxOutput);
            hwaParamCfg.source.srcBcnt = numAzimuthBins - 1;
        }

        hwaParamCfg.source.srcRealComplex = HWA_SAMPLES_FORMAT_REAL;
        hwaParamCfg.source.srcWidth       = HWA_SAMPLES_WIDTH_32BIT;
        hwaParamCfg.source.srcSign        = HWA_SAMPLES_UNSIGNED;
        hwaParamCfg.source.srcConjugate   = 0; // no conjugate
        hwaParamCfg.source.srcScale       = 0;

        hwaParamCfg.complexMultiply.mode                   = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
        hwaParamCfg.accelModeArgs.fftMode.fftEn            = 0;
        hwaParamCfg.accelModeArgs.fftMode.fftSize          = 0;
        hwaParamCfg.accelModeArgs.fftMode.windowEn         = 0;
        hwaParamCfg.accelModeArgs.fftMode.butterflyScaling = 0; // ToDo Tweak this
        hwaParamCfg.accelModeArgs.fftMode.fftOutMode       = HWA_FFT_MODE_OUTPUT_MAX_STATS;
        if (cfg->staticCfg.isDetMatrixLogScale)
        {
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_ENABLED;
        }
        else
        {
            hwaParamCfg.accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED;
        }


        hwaParamCfg.dest.dstAddr        = (uint16_t)DPU_DOAPROC_BANK_0 + 0x2000; // ToDo +0x2000 is temporary
        hwaParamCfg.dest.dstAcnt        = 4095; // HWA user guide recommendation
        hwaParamCfg.dest.dstAIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstBIdx        = 8; // HWA user guide recommendation
        hwaParamCfg.dest.dstRealComplex = 0; // HWA user guide recommendation
        hwaParamCfg.dest.dstWidth       = 1; // HWA user guide recommendation
        hwaParamCfg.dest.dstConjugate   = 0; // no conjugate
        hwaParamCfg.dest.dstScale       = 8;

        retVal = HWA_configParamSet(obj->hwaHandle, paramsetIdx, &hwaParamCfg, NULL);
        if (retVal != 0)
        {
            goto exit;
        }
        retVal = HWA_disableParamSetInterrupt(obj->hwaHandle,
                                              paramsetIdx,
                                              HWA_PARAMDONE_INTERRUPT_TYPE_DMA | HWA_PARAMDONE_INTERRUPT_TYPE_CPU);
        if (retVal != 0)
        {
            goto exit;
        }

        retVal = HWA_getDMAChanIndex(obj->hwaHandle,
                                     cfg->hwRes.edmaCfg.edmaDetMatOut.channel, // ToDo Add the correct output channel
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

        obj->hwaParamStopIdx = paramsetIdx;
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
 *  \ingroup    DPU_DOA_INTERNAL_FUNCTION
 *
 *  @retval EDMA error code, see EDMA API.
 */
static inline int32_t doaProc_configEdma(
    DPU_DoaProc_Obj    *obj,
    DPU_DoaProc_Config *cfg)
{
    int32_t            retVal            = SystemP_SUCCESS;
    cmplx16ImRe_t     *radarCubeBase     = (cmplx16ImRe_t *)cfg->hwRes.radarCube.data;
    uint8_t           *detMatrixBase     = (uint8_t *)cfg->hwRes.detMatrix.data;
    uint16_t          *elevIdxMatrixBase = (uint16_t *)cfg->hwRes.elevationIndexMatrix.data;
    uint16_t          *dopIdxMatrixBase  = (uint16_t *)cfg->hwRes.dopplerIndexMatrix.data;
    int16_t            sampleLenInBytes  = sizeof(cmplx16ImRe_t);
    DPEDMA_ChainingCfg chainingCfg;
    DPEDMA_syncABCfg   syncABCfg;
    int16_t            detMatrixElemLenInBytes;
    uint32_t           numElevationBins;

    bool               isTransferCompletionEnabled            = false;
    bool               isIntermediateTransferInterruptEnabled = false;
    Edma_EventCallback transferCompletionCallbackFxn          = NULL;
    void              *transferCompletionCallbackFxnArg       = NULL;
    uint8_t            chainingLoopEdmaChannel;
    uint8_t            transferType;
    uint32_t           detMatOutAddrOffset;
    uint32_t           numAzimuthBins;
    uint32_t           numOutputDopplerBins;

    if (obj == NULL)
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }

    if (cfg->staticCfg.isStaticClutterRemovalEnabled)
    {
        numOutputDopplerBins = cfg->staticCfg.numDopplerBins - 1;
    }
    else
    {
        numOutputDopplerBins = cfg->staticCfg.numDopplerBins;
    }

    if (cfg->staticCfg.isDetMatrixLogScale)
    {
        detMatrixElemLenInBytes = sizeof(uint16_t);
    }
    else
    {
        detMatrixElemLenInBytes = sizeof(uint32_t);
    }

    if (cfg->staticCfg.angleDimension == 2)
    {
        numAzimuthBins   = cfg->staticCfg.azimuthFftSize;
        numElevationBins = cfg->staticCfg.elevationFftSize;
    }
    else if (cfg->staticCfg.angleDimension == 1)
    {
        numAzimuthBins   = cfg->staticCfg.azimuthFftSize;
        numElevationBins = 1;
    }
    else if (cfg->staticCfg.angleDimension == 0)
    {
        numAzimuthBins   = 1;
        numElevationBins = 1;
    }
    else
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }


    /*****************************************************************************************/
    /**************                     PROGRAM DMA INPUT                    *****************/
    /*****************************************************************************************/
    /******************************************************************************************
     *  PROGRAM DMA channel  to transfer chunk[0] of data from Radar cube to HWA input buffer
     *  Currently only chunk[0] s used. Size of chunk[1] is zero.
     ******************************************************************************************/
    chainingCfg.chainingChan                  = cfg->hwRes.edmaCfg.edmaHotSig.channel;
    chainingCfg.isIntermediateChainingEnabled = true;
    chainingCfg.isFinalChainingEnabled        = true;


    syncABCfg.srcAddress = (uint32_t)(&radarCubeBase[0]);
    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        syncABCfg.destAddress = (uint32_t)cfg->hwRes.hwaCfg.hwaMemInpAddr;
    }
    else
    {
        syncABCfg.destAddress = (uint32_t)(obj->hwaMemBankAddr[0]);
    }
    syncABCfg.aCount  = sampleLenInBytes;
    syncABCfg.bCount  = cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * cfg->staticCfg.numDopplerChirps;
    syncABCfg.cCount  = cfg->staticCfg.numRangeBins;
    syncABCfg.srcBIdx = cfg->staticCfg.numRangeBins * sampleLenInBytes;
    syncABCfg.srcCIdx = sampleLenInBytes;
    syncABCfg.dstBIdx = sampleLenInBytes;
    syncABCfg.dstCIdx = 0;

    retVal = DPEDMA_configSyncAB(cfg->hwRes.edmaCfg.edmaHandle,
                                 &cfg->hwRes.edmaCfg.edmaIn.chunk[0],
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

    /******************************************************************************************
     *  PROGRAM DMA Hot Signature
     ******************************************************************************************/
    retVal = DPEDMAHWA_configOneHotSignature(cfg->hwRes.edmaCfg.edmaHandle,
                                             &cfg->hwRes.edmaCfg.edmaHotSig,
                                             obj->hwaHandle,
                                             obj->hwaDmaTriggerSourceChan,
                                             false);

    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************/
    /**************                      PROGRAM DMA OUTPUT                   *****************/
    /******************************************************************************************/

    chainingCfg.isIntermediateChainingEnabled = true;
    chainingLoopEdmaChannel                   = cfg->hwRes.edmaCfg.edmaIn.chunk[0].channel;
    transferType                              = 1;

    /****************************************************************************************/
    /* 2D case: (MAX (1)): [Det Matrix] -> [ElevInd Matrix] -> [DopplerInd Matrix]          */
    /* 2D case: (SUM (2)): [Det Matrix] -> [ElevInd Matrix] -> [DopplerInd Matrix]          */
    /* 2D case: (SUM (0)): [Det Matrix] -> [ElevInd Matrix]                     ToDo        */

    /* 1D case: (MAX (1)): [Det Matrix] -> [DopplerInd Matrix]                              */
    /* 1D case: (SUM (2)): [Det Matrix] -> [DopplerInd Matrix]                              */
    /* 1D case: (SUM (0)): [Det Matrix]                                         ToDo        */
    /****************************************************************************************
     *  PROGRAM DMA channel to transfer Detection Matrix to L3
     ****************************************************************************************/
    isIntermediateTransferInterruptEnabled = false;
    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim) && (cfg->staticCfg.angleDimension == 2))
    {
        /* (2D & MAX) */
        chainingCfg.chainingChan           = cfg->hwRes.edmaCfg.elevIndMatOut.channel;
        chainingCfg.isFinalChainingEnabled = true;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled            = false;
        transferCompletionCallbackFxn          = NULL;
        transferCompletionCallbackFxnArg       = NULL;
    }
    else if ((!cfg->staticCfg.selectCoherentPeakInDopplerDim) && (cfg->staticCfg.angleDimension == 2))
    {
        /* (2D & SUM) */
        chainingCfg.chainingChan           = cfg->hwRes.edmaCfg.elevIndMatOut.channel;
        chainingCfg.isFinalChainingEnabled = true;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled            = false;
        transferCompletionCallbackFxn          = NULL;
        transferCompletionCallbackFxnArg       = NULL;
    }
    else if ((cfg->staticCfg.selectCoherentPeakInDopplerDim) && (cfg->staticCfg.angleDimension <= 1))
    {
        /* (1D & MAX) */
        chainingCfg.chainingChan           = cfg->hwRes.edmaCfg.dopIndMatOut.channel;
        chainingCfg.isFinalChainingEnabled = true;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled            = false;
        transferCompletionCallbackFxn          = NULL;
        transferCompletionCallbackFxnArg       = NULL;
    }
    else if ((!cfg->staticCfg.selectCoherentPeakInDopplerDim) && (cfg->staticCfg.angleDimension <= 1))
    {
        /* (1D & SUM) */
        chainingCfg.chainingChan           = chainingLoopEdmaChannel;
        chainingCfg.isFinalChainingEnabled = false;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled            = true;
        transferCompletionCallbackFxn          = doaProc_edmaDoneIsrCallback;
        transferCompletionCallbackFxnArg       = (void *)&(obj->edmaDoneSemaHandle);
    }

    if (cfg->staticCfg.selectCoherentPeakInDopplerDim == 1)
    {
        detMatOutAddrOffset = 0x2000;
    }
    else if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 0) ||
             (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        detMatOutAddrOffset = 0x0000;
    }

    if (cfg->staticCfg.dopElevDimReductOrder == 0)
    {
        /* V1: Dimensionality reduction order: Doppler then Elevation */
        if (cfg->staticCfg.angleDimension == 2)
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[0] + sizeof(uint32_t) + 0x2000; // ToDo +0x2000 is temporary
        }
        else
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[2] + sizeof(uint32_t) + detMatOutAddrOffset; // ToDo +0x2000 is temporary
        }
    }
    else
    {
        /* V2: Dimensionality reduction order: Elevation then Doppler */
        if (cfg->staticCfg.angleDimension == 2)
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[1] + sizeof(uint32_t) + detMatOutAddrOffset; // ToDo +0x2000 is temporary
        }
        else
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[2] + sizeof(uint32_t) + detMatOutAddrOffset; // ToDo +0x2000 is temporary
        }
    }
    syncABCfg.destAddress = (uint32_t)(detMatrixBase);
    syncABCfg.aCount      = detMatrixElemLenInBytes;
    syncABCfg.bCount      = numAzimuthBins;
    syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
    syncABCfg.srcBIdx     = sizeof(DPU_DoaProc_HwaMaxOutput);
    syncABCfg.dstCIdx     = numAzimuthBins * detMatrixElemLenInBytes;
    syncABCfg.srcCIdx     = 0;
    syncABCfg.dstBIdx     = detMatrixElemLenInBytes;

    retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                       &cfg->hwRes.edmaCfg.edmaDetMatOut,
                                       &chainingCfg,
                                       &syncABCfg,
                                       true, // isEventTriggered
                                       isIntermediateTransferInterruptEnabled,
                                       isTransferCompletionEnabled,
                                       transferCompletionCallbackFxn,
                                       transferCompletionCallbackFxnArg,
                                       cfg->hwRes.edmaCfg.intrObj,
                                       transferType);
    if (retVal != SystemP_SUCCESS)
    {
        goto exit;
    }

    /******************************************************************************************
     *  PROGRAM DMA channel  to transfer Elevation Index Matrix to L3
     ******************************************************************************************/
    if (cfg->staticCfg.angleDimension == 2)
    {
        if (cfg->staticCfg.selectCoherentPeakInDopplerDim)
        {
            /* (2D & MAX) */
            chainingCfg.chainingChan           = cfg->hwRes.edmaCfg.dopIndMatOut.channel;
            chainingCfg.isFinalChainingEnabled = true;

            isIntermediateTransferInterruptEnabled = false;
            isTransferCompletionEnabled            = false;
            transferCompletionCallbackFxn          = NULL;
            transferCompletionCallbackFxnArg       = NULL;
        }
        else
        {
            /* (2D & SUM) */
            chainingCfg.chainingChan           = chainingLoopEdmaChannel;
            chainingCfg.isFinalChainingEnabled = false;

            isIntermediateTransferInterruptEnabled = false;
            isTransferCompletionEnabled            = true;
            transferCompletionCallbackFxn          = doaProc_edmaDoneIsrCallback;
            transferCompletionCallbackFxnArg       = (void *)&(obj->edmaDoneSemaHandle);
        }

        if (cfg->staticCfg.dopElevDimReductOrder == 0)
        {
            /* V1: Dimensionality reduction order: Doppler then Elevation */
            syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[0] + 0X2000;
            syncABCfg.destAddress = (uint32_t)(elevIdxMatrixBase);
            syncABCfg.aCount      = sizeof(uint8_t); // ToDo: We copy only one bytes elevation index
            syncABCfg.bCount      = numAzimuthBins;
            syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
            syncABCfg.srcBIdx     = sizeof(DPU_DoaProc_HwaMaxOutput);
            syncABCfg.dstCIdx     = numAzimuthBins * sizeof(uint8_t);
            syncABCfg.srcCIdx     = 0;
            syncABCfg.dstBIdx     = sizeof(uint8_t);

            retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                               &cfg->hwRes.edmaCfg.elevIndMatOut,
                                               &chainingCfg,
                                               &syncABCfg,
                                               false, // isEventTriggered
                                               isIntermediateTransferInterruptEnabled,
                                               isTransferCompletionEnabled,
                                               transferCompletionCallbackFxn,
                                               transferCompletionCallbackFxnArg,
                                               cfg->hwRes.edmaCfg.intrObj,
                                               transferType);
            if (retVal != SystemP_SUCCESS)
            {
                goto exit;
            }
        }
        else
        {
            /* V2: Dimensionality reduction order: Elevation then Doppler */
            syncABCfg.srcAddress  = (uint32_t)obj->hwaMemBankAddr[2];
            syncABCfg.destAddress = (uint32_t)(elevIdxMatrixBase);
            syncABCfg.aCount      = sizeof(uint8_t); // ToDo: We copy only one bytes elevation index
            syncABCfg.bCount      = numAzimuthBins * numOutputDopplerBins;
            syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
            syncABCfg.srcBIdx     = sizeof(DPU_DoaProc_HwaMaxOutput);
            syncABCfg.dstCIdx     = numAzimuthBins * numOutputDopplerBins * sizeof(uint8_t);
            syncABCfg.srcCIdx     = 0;
            syncABCfg.dstBIdx     = sizeof(uint8_t);

            retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                               &cfg->hwRes.edmaCfg.elevIndMatOut,
                                               &chainingCfg,
                                               &syncABCfg,
                                               false, // isEventTriggered
                                               isIntermediateTransferInterruptEnabled,
                                               isTransferCompletionEnabled,
                                               transferCompletionCallbackFxn,
                                               transferCompletionCallbackFxnArg,
                                               cfg->hwRes.edmaCfg.intrObj,
                                               transferType);
            if (retVal != SystemP_SUCCESS)
            {
                goto exit;
            }
        }
    }

    if ((cfg->staticCfg.selectCoherentPeakInDopplerDim == 1) ||
        (cfg->staticCfg.selectCoherentPeakInDopplerDim == 2))
    {
        /******************************************************************************************
         *  PROGRAM DMA channel  to transfer Doppler Index Matrix to L3
         ******************************************************************************************/
        /* (2D & MAX) */
        chainingCfg.chainingChan           = chainingLoopEdmaChannel;
        chainingCfg.isFinalChainingEnabled = false;

        isIntermediateTransferInterruptEnabled = false;
        isTransferCompletionEnabled            = true;
        transferCompletionCallbackFxn          = doaProc_edmaDoneIsrCallback;
        transferCompletionCallbackFxnArg       = (void *)&(obj->edmaDoneSemaHandle);

        /****************************************************************************************/
        /* 2D case: (MAX): [Det Matrix] -> [ElevInd Matrix] -> [DopplerInd Matrix]              */
        /* 1D case: (MAX): [Det Matrix] -> [DopplerInd Matrix]                                  */
        /****************************************************************************************/
        if (cfg->staticCfg.angleDimension == 2)
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[1] + 0x2000; // ToDo +0x2000 is temporary
        }
        else
        {
            syncABCfg.srcAddress = (uint32_t)obj->hwaMemBankAddr[2] + 0x2000; // ToDo +0x2000 is temporary
        }

        if (cfg->staticCfg.dopElevDimReductOrder == 0)
        {
            /* V1: Dimensionality reduction order: Doppler then Elevation */
            syncABCfg.destAddress = (uint32_t)(dopIdxMatrixBase);
            syncABCfg.aCount      = sizeof(uint8_t); // ToDo: copy only one byte at a time, check if it is more efficient to copy everything
            syncABCfg.bCount      = numAzimuthBins * numElevationBins;
            syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
            syncABCfg.srcBIdx     = sizeof(DPU_DoaProc_HwaMaxOutput);
            syncABCfg.dstCIdx     = numAzimuthBins * numElevationBins * sizeof(uint8_t);
            syncABCfg.srcCIdx     = 0;
            syncABCfg.dstBIdx     = sizeof(uint8_t);
        }
        else
        {
            /* V1: Dimensionality reduction order: Doppler then Elevation */
            syncABCfg.destAddress = (uint32_t)(dopIdxMatrixBase);
            syncABCfg.aCount      = sizeof(uint8_t); // ToDo: copy only one byte at a time, check if it is more efficient to copy everything
            syncABCfg.bCount      = numAzimuthBins;
            syncABCfg.cCount      = cfg->staticCfg.numRangeBins;
            syncABCfg.srcBIdx     = sizeof(DPU_DoaProc_HwaMaxOutput);
            syncABCfg.dstCIdx     = numAzimuthBins * sizeof(uint8_t);
            syncABCfg.srcCIdx     = 0;
            syncABCfg.dstBIdx     = sizeof(uint8_t);
        }

        retVal = DPEDMA_configSyncTransfer(cfg->hwRes.edmaCfg.edmaHandle,
                                           &cfg->hwRes.edmaCfg.dopIndMatOut,
                                           &chainingCfg,
                                           &syncABCfg,
                                           false, // isEventTriggered
                                           isIntermediateTransferInterruptEnabled,
                                           isTransferCompletionEnabled,
                                           transferCompletionCallbackFxn,
                                           transferCompletionCallbackFxnArg,
                                           cfg->hwRes.edmaCfg.intrObj,
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
 *  \ingroup    DPU_DOAPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - valid handle
 *  @retval
 *      Error       - NULL
 */
DPU_DoaProc_Handle DPU_DoaProc_init(
    DPU_DoaProc_InitParams *initCfg,
    int32_t                *errCode)
{
    DPU_DoaProc_Obj *obj = NULL;
    HWA_MemInfo      hwaMemInfo;
    uint32_t         i;
    int32_t          status = SystemP_SUCCESS;

    *errCode = 0;

    if ((initCfg == NULL) || (initCfg->hwaHandle == NULL))
    {
        *errCode = DPU_DOAPROC_EINVAL;
        goto exit;
    }

    /* Allocate memory */
    obj = (DPU_DoaProc_Obj *)&gDoaProcHeapMem;
    if (obj == NULL)
    {
        *errCode = DPU_DOAPROC_ENOMEM;
        goto exit;
    }

    /* Initialize memory */
    memset((void *)obj, 0U, sizeof(DPU_DoaProc_Obj));

    // RPMF printf("DOA DPU: (DPU_DoaProc_Obj *) 0x%08x\n", (uint32_t) obj);

    /* Save init config params */
    obj->hwaHandle = initCfg->hwaHandle;

    /* Create DPU semaphores */
    status = SemaphoreP_constructBinary(&obj->edmaDoneSemaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_DOAPROC_ESEMA;
        goto exit;
    }

    status = SemaphoreP_constructBinary(&obj->hwaDoneSemaHandle, 0);
    if (SystemP_SUCCESS != status)
    {
        *errCode = DPU_DOAPROC_ESEMA;
        goto exit;
    }

    /* Populate HWA base addresses and offsets. This is done only once, at init time.*/
    *errCode = HWA_getHWAMemInfo(obj->hwaHandle, &hwaMemInfo);
    if (*errCode < 0)
    {
        goto exit;
    }

    /* check if we have enough memory banks*/
    if (hwaMemInfo.numBanks < DPU_DOAPROC_NUM_HWA_MEMBANKS)
    {
        *errCode = DPU_DOAPROC_EHWARES;
        goto exit;
    }

    for (i = 0; i < DPU_DOAPROC_NUM_HWA_MEMBANKS; i++)
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
    return ((DPU_DoaProc_Handle)obj);
}

/**
 *  @b Description
 *  @n
 *   DOA DPU configuration
 *
 *  @param[in]   handle     DPU handle.
 *  @param[in]   cfg        Pointer to configuration parameters.
 *
 *  \ingroup    DPU_DOAPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success      = 0
 *  @retval
 *      Error       != 0 @ref DPU_DOPPLERPROC_ERROR_CODE
 */
int32_t DPU_DoaProc_config(
    DPU_DoaProc_Handle  handle,
    DPU_DoaProc_Config *cfg)
{
    DPU_DoaProc_Obj *obj;
    int32_t          retVal = 0;
    uint32_t         numElevationBins;
    uint32_t         numAzimuthBins;


    obj = (DPU_DoaProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }

    if (cfg->staticCfg.angleDimension == 2)
    {
        numAzimuthBins   = cfg->staticCfg.azimuthFftSize;
        numElevationBins = cfg->staticCfg.elevationFftSize;
    }
    else if (cfg->staticCfg.angleDimension == 1)
    {
        numAzimuthBins   = cfg->staticCfg.azimuthFftSize;
        numElevationBins = 1;
    }
    else
    {
        numAzimuthBins   = 1;
        numElevationBins = 1;
    }

#if DEBUG_CHECK_PARAMS
    /* Validate params */
    if (!cfg ||
        !cfg->hwRes.edmaCfg.edmaHandle ||
        !cfg->hwRes.hwaCfg.window
        //! cfg->hwRes.radarCube.data
    )
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }

    /* Check if radar cube format is supported by DPU*/
    if (cfg->hwRes.radarCube.datafmt != DPIF_RADARCUBE_FORMAT_6)
    {
        retVal = DPU_DOAPROC_ECUBEFORMAT;
        goto exit;
    }

    /* Check if radar cube column fits into one HWA memory bank */
    if ((cfg->staticCfg.numTxAntennas * cfg->staticCfg.numRxAntennas *
         cfg->staticCfg.numDopplerChirps * sizeof(cmplx16ImRe_t)) > (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS))
    {
        retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
        goto exit;
    }

    /* Check if Azimuth FFT output fits into Two HWA memory banks */
    {
        uint32_t outputElementSizeBytes;
        if (cfg->staticCfg.angleDimension == 2)
        {
            outputElementSizeBytes = sizeof(cmplx32ImRe_t);
        }
        else
        {
            outputElementSizeBytes = sizeof(uint32_t);
        }
        if ((cfg->staticCfg.numAntRow * cfg->staticCfg.numDopplerChirps * numAzimuthBins *
             outputElementSizeBytes) > 2 * (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS))
        {
            retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }
    if (cfg->staticCfg.dopElevDimReductOrder == 0)
    {
        /* V1: Dimensionality reduction order: Doppler then Elevation */
        /* Check if the elevation magnitude output (uint32_t) fits into Two HWA memory banks. */
        if ((numAzimuthBins * numElevationBins *
             cfg->staticCfg.numDopplerBins * sizeof(uint32_t)) > 2 * (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS))
        {
            retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
            goto exit;
        }
        /* Check if the Doppler Max/SUM param output (uint32_t) fits into one half of the HWA memory bank. */
        if ((numAzimuthBins * numElevationBins * sizeof(DPU_DoaProc_HwaMaxOutput)) > ((SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS) / 2))
        {
            retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }
    else
    {
        /* V2: Dimensionality reduction order: Elevation then Doppler */
        /* Check if the elevation output fits into Two HWA memory banks. */
        if ((numAzimuthBins * cfg->staticCfg.numDopplerBins * sizeof(DPU_DoaProc_HwaMaxOutput)) > 2 * (SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS))
        {
            retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
            goto exit;
        }
        /* Check if the Doppler Max/SUM param output (uint32_t) fits into one half of the HWA memory bank. */
        if ((numAzimuthBins * sizeof(DPU_DoaProc_HwaMaxOutput)) > ((SOC_HWA_MEM_SIZE / SOC_HWA_NUM_MEM_BANKS) / 2))
        {
            retVal = DPU_DOAPROC_EEXCEEDHWAMEM;
            goto exit;
        }
    }

    if (cfg->staticCfg.numDopplerBins > 256)
    {
        /*Currently it is limited to 256, since the doppler maximum index is stored in array of type uint8_t. */
        retVal = DPU_DOAPROC_E_EXCEEDED_MAX_NUM_DOPPLER_BINS;
        goto exit;
    }
#endif

    /* Save necessary parameters to DPU object that will be used during Process time */
    /* EDMA parameters needed to trigger first EDMA transfer*/
    obj->edmaHandle = cfg->hwRes.edmaCfg.edmaHandle;
    memcpy((void *)(&obj->edmaIn), (void *)(&cfg->hwRes.edmaCfg.edmaIn), sizeof(DPU_DoaProc_Edma));
    memcpy((void *)(&obj->edmaDetMatOut), (void *)(&cfg->hwRes.edmaCfg.edmaDetMatOut), sizeof(DPEDMA_ChanCfg));
    memcpy((void *)(&obj->edmaInterLoopIn), (void *)(&cfg->hwRes.edmaCfg.edmaInterLoopIn), sizeof(DPEDMA_ChanCfg));

    /*HWA parameters needed for the HWA common configuration*/
    obj->hwaNumLoops      = cfg->staticCfg.numRangeBins;
    obj->hwaParamStartIdx = cfg->hwRes.hwaCfg.paramSetStartIdx;
    obj->hwaParamStopIdx  = cfg->hwRes.hwaCfg.paramSetStartIdx + cfg->hwRes.hwaCfg.numParamSets - 1;
    // obj->doaRangeLoopType = cfg->staticCfg.doaRangeLoopType;

    obj->numVirtualAntennas                 = cfg->staticCfg.numVirtualAntennas;
    obj->compRxChanCfg                      = cfg->staticCfg.compRxChanCfg; // Copy structure
    obj->isRxChGainPhaseCompensationEnabled = cfg->staticCfg.isRxChGainPhaseCompensationEnabled;

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /* HWA window configuration */
    retVal = HWA_configRam(obj->hwaHandle,
                           HWA_RAM_TYPE_WINDOW_RAM,
                           (uint8_t *)cfg->hwRes.hwaCfg.window,
                           cfg->hwRes.hwaCfg.windowSize, // size in bytes
                           cfg->hwRes.hwaCfg.winRamOffset * sizeof(int32_t));
    if (retVal != 0)
    {
        goto exit;
    }

    if (cfg->staticCfg.isRxChGainPhaseCompensationEnabled)
    {
        /* HWA window configuration */
        retVal = HWA_configRam(obj->hwaHandle,
                               HWA_RAM_TYPE_INTERNAL_RAM,
                               (uint8_t *)cfg->staticCfg.compRxChanCfg.rxChPhaseComp,
                               cfg->staticCfg.numRxAntennas * cfg->staticCfg.numTxAntennas * sizeof(cmplx32ImRe_t), // size in bytes
                               0);
        if (retVal != 0)
        {
            goto exit;
        }
    }

    /*******************************/
    /**  Configure HWA            **/
    /*******************************/
    obj->hwaDmaTriggerSourceChan = cfg->hwRes.hwaCfg.dmaTrigSrcChan;
    if (cfg->staticCfg.dopElevDimReductOrder == 0)
    {
        retVal = doaProc_configHwa(obj, cfg);
        if (retVal != 0)
        {
            goto exit;
        }
    }
    else
    {
        retVal = doaProc_configHwa_v2(obj, cfg);
        if (retVal != 0)
        {
            goto exit;
        }
    }
    /*******************************/
    /**  Configure EDMA           **/
    /*******************************/
    retVal = doaProc_configEdma(obj, cfg);
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
 *  \ingroup    DPU_DOAPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t doaProc_setInputEdma(DPU_DoaProc_Obj             *obj,
                             DPU_DoaProc_RadarCubeSource *radarCubeSrc)
{
    int32_t  retVal = 0;
    uint32_t baseAddr;

    baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    DebugP_assert(baseAddr != 0);

    retVal = DPEDMA_setSourceAddress(obj->edmaHandle,
                                     obj->edmaIn.chunk[0].channel,
                                     (uint32_t)SOC_virtToPhy((void *)radarCubeSrc->chunk[0].srcAddress));
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
 *  \ingroup    DPU_DOAPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t doaProc_setOutputEdma(DPU_DoaProc_Obj *obj,
                              DPIF_DetMatrix  *detMatrix)
{
    int32_t retVal = 0;

    retVal = DPEDMA_setDestinationAddress(obj->edmaHandle,
                                          (uint8_t)obj->edmaDetMatOut.channel,
                                          (uint32_t)SOC_virtToPhy(detMatrix->data));
    return retVal;
}

/**
 *  @b Description
 *  @n DOA DPU process function. Computes the range/azimuth detection matrix
 *
 *  @param[in]   handle        DPU handle
 *  @param[in]   radarCubeSrc  Structure descriptor of the input radar cube
 *  @param[in]   detMatrix     Pointer to output detection matrix.
 *  @param[out]  outParams     Output parameters.
 *
 *  \ingroup    DPU_DOAPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t DPU_DoaProc_process(
    DPU_DoaProc_Handle           handle,
    DPU_DoaProc_RadarCubeSource *radarCubeSrc,
    DPIF_DetMatrix              *detMatrix,
    DPU_DoaProc_OutParams       *outParams)
{
    // volatile uint32_t   startTime;

    DPU_DoaProc_Obj *obj;
    int32_t          retVal = 0;

    obj = (DPU_DoaProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }
    /* Set inProgress state */
    obj->inProgress = true;

    // startTime = Cycleprofiler_getTimeStamp();

    /* Configure Input EDMA */
    doaProc_setInputEdma(obj, radarCubeSrc);

    /* Configure Output EDMA */
    doaProc_setOutputEdma(obj, detMatrix);

    doaProc_InternalLoop(obj, outParams);

    outParams->stats.numProcess++;
    // outParams->stats.processingTime = Cycleprofiler_getTimeStamp() - startTime;

exit:
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
 *  \ingroup    DPU_DOAPROC_EXTERNAL_FUNCTION
 *
 *  @retval
 *      Success      =0
 *  @retval
 *      Error       !=0 @ref DPU_DOPPLERPROC_ERROR_CODE
 */
int32_t DPU_DoaProc_deinit(DPU_DoaProc_Handle handle)
{
    int32_t retVal = 0;

    /* Free memory */
    if (handle == NULL)
    {
        retVal = DPU_DOAPROC_EINVAL;
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
int32_t DPU_DoaProc_GetNumUsedHwaParamSets(
    DPU_DoaProc_Handle handle,
    uint8_t           *numUsedHwaParamSets)
{
    DPU_DoaProc_Obj *obj;
    int32_t          retVal = 0;

    obj = (DPU_DoaProc_Obj *)handle;
    if (obj == NULL)
    {
        retVal = DPU_DOAPROC_EINVAL;
        goto exit;
    }
    *numUsedHwaParamSets = (uint8_t)(obj->hwaParamStopIdx - obj->hwaParamStartIdx + 1);
exit:
    return retVal;
}

/**
 *  @b Description
 *  @n DOA DPU process function using internal HWA loop
 *
 *  @param[in]   obj        DPU object
 *  @param[out]  outParams     Output parameters.
 *
 *  \ingroup    DPU_DOAPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     =0
 *  @retval
 *      Error      !=0
 */
int32_t doaProc_InternalLoop(
    DPU_DoaProc_Obj       *obj,
    DPU_DoaProc_OutParams *outParams)
{

    int32_t retVal = 0;
    // int32_t             i, j;
    bool             status;
    HWA_CommonConfig hwaCommonConfig;
    uint32_t         baseAddr, regionId;
    // uint32_t *hwaRdStatus = (uint32_t *)(0x5501009C);

    baseAddr = EDMA_getBaseAddr(obj->edmaHandle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(obj->edmaHandle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    if (obj->isRxChGainPhaseCompensationEnabled)
    {
        /* HWA window configuration */
        retVal = HWA_configRam(obj->hwaHandle,
                               HWA_RAM_TYPE_INTERNAL_RAM,
                               (uint8_t *)obj->compRxChanCfg.rxChPhaseComp,
                               obj->numVirtualAntennas * sizeof(cmplx32ImRe_t), // size in bytes
                               0);
        if (retVal != 0)
        {
            goto exit;
        }
    }

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    retVal = HWA_enableDoneInterrupt(obj->hwaHandle,
                                     doaProc_hwaDoneIsrCallback,
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

    hwaCommonConfig.numLoops                        = obj->hwaNumLoops;
    hwaCommonConfig.paramStartIdx                   = obj->hwaParamStartIdx;
    hwaCommonConfig.paramStopIdx                    = obj->hwaParamStopIdx;
    hwaCommonConfig.fftConfig.fft1DEnable           = HWA_FEATURE_BIT_DISABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;

    hwaCommonConfig.fftConfig.fftSumDiv = obj->dopFftSumDiv;

    hwaCommonConfig.scalarMult.i_cmult_scale[0] = 0;
    hwaCommonConfig.scalarMult.q_cmult_scale[0] = 0;


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

    EDMAEnableTransferRegion(baseAddr, regionId, obj->edmaIn.chunk[0].channel, EDMA_TRIG_MODE_MANUAL); // run param

    /**********************************************/
    /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
    /**********************************************/
    status = SemaphoreP_pend(&obj->hwaDoneSemaHandle, SystemP_WAIT_FOREVER);

    if (status != SystemP_SUCCESS)
    {
        retVal = DPU_DOAPROC_ESEMASTATUS;
        goto exit;
    }

    HWA_disableDoneInterrupt(obj->hwaHandle);

    /* Disable the HWA */
    retVal = HWA_enable(obj->hwaHandle, 0);
    if (retVal != 0)
    {
        goto exit;
    }

    /**********************************************/
    /* WAIT FOR EDMA DONE INTERRUPT            */
    /**********************************************/
    status = SemaphoreP_pend(&obj->edmaDoneSemaHandle, SystemP_WAIT_FOREVER);
    if (status != SystemP_SUCCESS)
    {
        retVal = DPU_DOAPROC_ESEMASTATUS;
        goto exit;
    }
exit:
    return retVal;
}
