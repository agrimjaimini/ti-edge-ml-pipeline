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
#ifndef AOASVCPROC_HWA_H
#define AOASVCPROC_HWA_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Driver/Common Include Files */
#include <drivers/hwa.h>
#include <common/syscommon.h>

/* DPIF Components Include Files */
#include <dpif/dpif_detmatrix.h>
#include <dpif/dpif_radarcube.h>

/* mmWave SDK Data Path Include Files */
#include <dpif/dp_error.h>
#include <dpif/dpif_pointcloud.h>
#include <aoasvcproc/v1/aoasvcproccommon.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup DPU_AOASVCPROC_ERROR_CODE
 *  Base error code for the dopplerProc DPU is defined in the
 *  \include dpif/dp_error.h
 @{ */

/**
 * @brief   Error Code: Invalid argument
 */
#define DPU_AOASVCPROC_EINVAL (DP_ERRNO_AOASVC_PROC_BASE - 1)

/**
 * @brief   Error Code: Out of memory
 */
#define DPU_AOASVCPROC_ENOMEM (DP_ERRNO_AOASVC_PROC_BASE - 2)

/**
 * @brief   Error Code: DPU is in progress
 */
#define DPU_AOASVCPROC_EINPROGRESS (DP_ERRNO_AOASVC_PROC_BASE - 3)

/**
 * @brief   Error Code: Out of HWA resources
 */
#define DPU_AOASVCPROC_EHWARES (DP_ERRNO_AOASVC_PROC_BASE - 4)

/**
 * @brief   Error Code: Semaphore creation failed
 */
#define DPU_AOASVCPROC_ESEMA (DP_ERRNO_AOASVC_PROC_BASE - 5)

/**
 * @brief   Error Code: Bad semaphore status
 */
#define DPU_AOASVCPROC_ESEMASTATUS (DP_ERRNO_AOASVC_PROC_BASE - 6)

/**
 * @brief   Error Code: Configure parameters exceed HWA memory bank size
 */
#define DPU_AOASVCPROC_EEXCEEDHWAMEM (DP_ERRNO_AOASVC_PROC_BASE - 7)

/**
 * @brief   Error Code: Unsupported radar cube format
 */
#define DPU_AOASVCPROC_ECUBEFORMAT (DP_ERRNO_AOASVC_PROC_BASE - 8)

/**
 * @brief   Error Code: Unsupported detection matrix format
 */
#define DPU_AOASVCPROC_EDETMFORMAT (DP_ERRNO_AOASVC_PROC_BASE - 9)

/**
 * @brief   Error Code: Insufficient detection matrix size
 */
#define DPU_AOASVCPROC_EDETMSIZE (DP_ERRNO_AOASVC_PROC_BASE - 10)

/**
 * @brief   Error Code: Wrong window size
 */
#define DPU_AOASVCPROC_EWINDSIZE (DP_ERRNO_AOASVC_PROC_BASE - 11)

/**
 * @brief   Error Code: Not supported angle dim 1 (no elevation) and
 */
#define DPU_AOASVCPROC_E_NOTSUPPORTED_ANGLE_DIM_ONE_AND_CPU_LOOP_CTRL (DP_ERRNO_AOASVC_PROC_BASE - 12)

/**
 * @brief   Error Code: Doppler FFT size exceeded the limit. Maximum Doppler index is stored in array of type uint8_t.
 */
#define DPU_AOASVCPROC_E_EXCEEDED_MAX_NUM_DOPPLER_BINS (DP_ERRNO_AOASVC_PROC_BASE - 13)

/**
@}
*/

/**
 * @brief   Maximum number of HWA paramsets used by DPU.
 */
#define DPU_AOASVCPROC_MAX_NUM_HWA_PARAMSET 4

/**
 * @brief   Number of HWA memory banks needed
 */
#define DPU_AOASVCPROC_NUM_HWA_MEMBANKS 4

/**
 * @brief   Disables first butterfly stage scaling
 */
#define DPU_AOASVCPROC_FIRST_SCALING_DISABLED ((uint8_t)0U)

/**
 * @brief   Enables first butterfly stage scaling
 */
#define DPU_AOASVCPROC_FIRST_SCALING_ENABLED ((uint8_t)1U)

/**
 * @brief   Range loop type controlled by HWA
 */
#define DPU_AOASVCPROC_RANGE_LOOP_HWA_CONTROL (0)
/**
 * @brief   Range loop type controlled by CPU
 */
#define DPU_AOASVCPROC_RANGE_LOOP_CPU_CONTROL (1)


    /**
     * @brief
     *  Source and destination addresses for EDMA input configuration.
     *
     * @details
     *  The structure is one of the input arguments of the DPU's proces function.
     *
     *  \ingroup DPU_AOAPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_RadarCubeChunkEdmaCfg_t
    {
        /*! @brief     Address in L3 of the chunk of data in radar cube matrix to be copied to HWA  */
        uint32_t srcAddress;

        /*! @brief     Destination address in HWA of the copied chunk */
        uint32_t dstAddress;

        /*! @brief     Packed EDMA values for A count (LSB 16-bit position) and B count (MSB 16-bit position */
        uint32_t Bcnt_Acnt;
    } DPU_AoasvcProc_RadarCubeChunkEdmaCfg;

    typedef struct DPU_AoasvcProc_RadarCubeSource_t
    {
        DPU_AoasvcProc_RadarCubeChunkEdmaCfg chunk[2];
    } DPU_AoasvcProc_RadarCubeSource;

    /*!
     *  @brief   Handle for Doppler Processing DPU.
     */
    typedef void *DPU_AoasvcProc_Handle;

    /**
     * @brief
     *  dopplerProc DPU initial configuration parameters
     *
     * @details
     *  The structure is used to hold the DPU initial configurations.
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_InitCfg_t
    {
        HWA_Handle hwaHandle;

    } DPU_AoasvcProc_InitParams;

    /**
     * @brief
     *  Configuration parameters for each HWA param set performing the Doppler FFTs and mappng output into
     *  row-col array as input to azimuth/elevation FFTs
     *
     * @details
     * Input antenna format to Doppler FFTs per range bin is X[antInd][chirpInd]
     * Output antenna format is Y[dopplerInd][rowInd][colInd]
     * HWA param set can perform only fixed source and destination increment (B dimension)
     *
     */
    typedef struct DPU_AoasvcProc_HWA_Doppler_Fft_Cfg_t
    {
        /*! @brief   Number of input columns (doppler FFTs) in the input matrix to process */
        uint16_t srcBcnt;

        /*! @brief   Column offset in the input matrix */
        int16_t srcAddrOffset;

        /*! @brief  column increment in number of columns in source matrix */
        int16_t srcBidx;

        /*! @brief   Scale in pre compensation */
        int16_t scale;

        /*! @brief  column/row offset in the destination matrix dstAddrOffset = colOffset + rowOffset*numColumns */
        int16_t dstAddrOffset;

        /*! @brief  column increment in number of columns in destination matrix */
        int16_t dstBidx;
    } DPU_AoasvcProc_HWA_Doppler_Fft_Cfg;

/**
 * @brief    Maximum nuber of Doppler FFT params
 *
 */
#define DPU_AOASVC_PROC_MAX_NUM_DOP_FFFT_PARAMS 5

/**
 * @brief    Maximum number of antenna elements in 2D virtual antenna array (maximum value of the product numRows*numCol)
 *
 */
#define DPU_AOASVC_PROC_MAX_2D_ANT_ARRAY_ELEMENTS 16

    /**
     * @brief
     *
     */
    typedef struct DPU_AoasvcProc_HWA_Option_Cfg_t
    {
        uint8_t                            numDopFftParams;
        DPU_AoasvcProc_HWA_Doppler_Fft_Cfg dopFftCfg[DPU_AOASVC_PROC_MAX_NUM_DOP_FFFT_PARAMS];
    } DPU_AoasvcProc_HWA_Option_Cfg;

    /**
     * @brief
     *  dopplerProc DPU HWA configuration parameters
     *
     * @details
     *  The structure is used to hold the HWA configuration parameters
     *  for the Doppler Processing DPU
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_HwaCfg_t
    {
        /*! @brief Indicates if HWA window is symmetric or non-symmetric.
            Use HWA macro definitions for symmetric/non-symmetric.
        */
        uint8_t winSym;

        /*!  @brief Doppler FFT window size in bytes.
             This is the number of coefficients to be programmed in the HWA for the windowing
             functionality. The size is a function of numDopplerChirps as follows:\n
             If non-symmetric window is selected: windowSize = numDopplerChirps * sizeof(int32_t) \n
             If symmetric window is selected and numDopplerChirps is even:
             windowSize = numDopplerChirps * sizeof(int32_t) / 2 \n
             If symmetric window is selected and numDopplerChirps is odd:
             windowSize = (numDopplerChirps + 1) * sizeof(int32_t) / 2
        */
        uint32_t windowSize;

        /*! @brief Pointer to Doppler FFT window coefficients. */
        int32_t *window;

        /*! @brief HWA window RAM offset in number of samples. */
        uint32_t winRamOffset;

        /*! @brief Indicates if HWA should enable butterfly scaling (divide by 2) of the
             first radix-2 stage. Depending on the window definition,
             user may want to skip the first stage scaling in order to avoid signal degradation.\n
             Options are:\n
             Disable first stage scaling: firstStageScaling = @ref DPU_AOASVCPROC_FIRST_SCALING_DISABLED \n
             Enable first stage scaling: firstStageScaling = @ref DPU_AOASVCPROC_FIRST_SCALING_ENABLED \n
             Note: All other butterfly stages have the scaling enabled.
             This option applies only for the first stage.\n
        */
        uint8_t firstStageScaling;

        /*! @brief Number of HWA paramsets reserved for the Doppler DPU.
             The number of HWA paramsets required by this DPU is a function of the number of TX antennas
             used in the configuration:\n
             numParamSets = 2 x (Number of TX antennas) + 2\n
             The DPU will use numParamSets consecutively, starting from paramSetStartIdx.\n
        */
        uint8_t numParamSets;

        /*! @brief HWA paramset Start index.
             Application has to ensure that paramSetStartIdx is such that \n
            [paramSetStartIdx, paramSetStartIdx + 1, ... (paramSetStartIdx + numParamSets - 1)] \n
            is a valid set of HWA paramsets.\n
        */
        uint32_t paramSetStartIdx;

        /*! @brief HWA Doppler FFT Parmas set configuration (performing Doppler FFT and antenna mapping)
         */
        DPU_AoasvcProc_HWA_Option_Cfg aoasvcRngGateCfg;

        /*! @brief HWA param set dma trigger source channel
         */
        uint8_t dmaTrigSrcChan[2];
    } DPU_AoasvcProc_HwaCfg;

    /*!
     *  @brief    Maximum peaks filled by HWA statistics block
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef volatile struct DPU_AoasvcProc_HwaMaxOutput_t
    {
        uint32_t maxInd; /*!< @brief Maximum peak index position */
        uint32_t peak; /*!< @brief Maximum peak value */
    } DPU_AoasvcProc_HwaMaxOutput;

    /**
     * @brief
     *  Sum filled by HWA statistics block
     *
     */
    typedef volatile struct DPU_AoasvcProc_HwaSumOutput_t
    {
        int32_t sumIm; /*!< @brief Sum imaginary */
        int32_t sumRe; /*!< @brief Sum real */
    } DPU_AoasvcProc_HwaSumOutput;

    /**
     * @brief
     *  dopplerProc DPU EDMA configuration parameters
     *
     * @details
     *  The structure is used to hold the EDMA configuration parameters
     *  for the Doppler Processing DPU
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_EdmaCfg_t
    {
        /*! @brief  EDMA driver handle. */
        EDMA_Handle edmaHandle;

        /*! @brief  EDMA Instance id */
        uint32_t edmaInstanceId;

        /*! @brief  EDMA configuration for Input data */
        DPEDMA_ChanCfg edmaIn[2];

        /*! @brief  EDMA configuration for hot signature. */
        DPEDMA_ChanCfg edmaHotSig;

        /*! @brief  EDMA configuration for Output data */
        DPEDMA_ChanCfg edmaOut[2];

        /*! @brief  EDMA interrupt object. */
        Edma_IntrObject intrObj[2];

        /*! @brief  EDMA configuration for steering vectors transfer to HWA */
        DPEDMA_ChanCfg edmaInSteerVec;

        /*! @brief  EDMA interrupt object. */
        Edma_IntrObject intrObjSteerVecTransfer;

    } DPU_AoasvcProc_EdmaCfg;

    typedef struct DPU_AoasvcProc_VirtualAntennaElements_t
    {
        uint16_t      rangeInd;
        int16_t       dopInd;
        cmplx32ImRe_t antenna[SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];
    } DPU_AoasvcProc_VirtualAntennaElements;


    /**
     * @brief
     *  Doppler DPU HW configuration parameters
     *
     * @details
     *  The structure is used to hold the  HW configuration parameters
     *  for the Doppler DPU
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_HW_Resources_t
    {
        /*! @brief  EDMA configuration */
        DPU_AoasvcProc_EdmaCfg edmaCfg;

        /*! @brief  HWA configuration */
        DPU_AoasvcProc_HwaCfg hwaCfg;

        /*! @brief  Radar Cube */
        DPIF_RadarCube radarCube[2];

        /*! @brief  Detection matrix */
        DPIF_DetMatrix detMatrix;

        /*! @brief  Doppler index matrix */
        DPIF_DetMatrix dopplerIndexMatrix;

        /*! @brief  elevation index matrix */
        DPIF_DetMatrix elevationIndexMatrix;

        /*! @brief  For external range loop (CPU contolled) interloop buffer for Azimuth FFT output */
        cmplx32ImRe_t *interLoopDataBuffer;

        DPU_AoasvcProc_VirtualAntennaElements *virtAntElemList;
    } DPU_AoasvcProc_HW_Resources;

    /**
     * @brief Range Bias and rx channel gain/phase compensation configuration.
     *
     *
     *  \ingroup    DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPU_AoasvcProc_compRxChannelBiasCfg_t
    {

        /*! @brief  Compensation for range estimation bias in meters */
        float rangeBias;

        /*! @brief  Compensation for Rx channel phase bias in Q20 format.
         *          The order here is like x[tx][rx] where rx order is 0,1,....SYS_COMMON_NUM_RX_CHANNEL-1
         *          and tx order is 0, 1,...,SYS_COMMON_NUM_TX_ANTENNAS-1 */
        cmplx32ReIm_t rxChPhaseComp[SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];

    } DPU_AoasvcProc_compRxChannelBiasCfg;

    /**
     * @brief Range Bias and rx channel gain/phase compensation configuration.
     *
     *
     *  \ingroup    DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPU_AoasvcProc_compRxChannelBiasFloatCfg_t
    {

        /*! @brief  Compensation for range estimation bias in meters */
        float rangeBias;

        /*! @brief  Compensation for Rx channel phase bias in Q20 format.
         *          The order here is like x[tx][rx] where rx order is 0,1,....SYS_COMMON_NUM_RX_CHANNEL-1
         *          and tx order is 0, 1,...,SYS_COMMON_NUM_TX_ANTENNAS-1 */
        float rxChPhaseComp[2 * SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];

    } DPU_AoasvcProc_compRxChannelBiasFloatCfg;


    /**
     * @brief
     *  Doppler DPU static configuration parameters
     *
     * @details
     *  The structure is used to hold the static configuration parameters
     *  for the AOASVC DPU.
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_StaticConfig_t
    {
        /*! @brief  Number of transmit antennas */
        uint8_t numTxAntennas;

        /*! @brief  Number of receive antennas */
        uint8_t numRxAntennas;

        /*! @brief  Number of virtual antennas */
        uint8_t numVirtualAntennas;

        /*! @brief  Number of range bins */
        uint16_t numRangeBins;

        /*! @brief  Number of Doppler chirps. */
        uint16_t numDopplerChirps;

        /*! @brief  Number of Doppler bins */
        uint16_t numDopplerBins;

        /*! @brief  Azimuth FFT size */
        uint16_t azimuthFftSize;

        /*! @brief  elevation FFT size */
        uint16_t elevationFftSize;

        /*! @brief  Number of chirps per frame for Minor Motion Detection */
        uint16_t numMinorMotionChirpsPerFrame;

        /*! @brief  Number of frames Minor Motion Detection spreads over */
        uint16_t numFrmPerMinorMotProc;


        /*! @brief  Log2 of number of Doppler bins */
        uint8_t log2NumDopplerBins;

        /*! @brief     true - select the coherent peak in Doppler, false - non-coherently combine accross Doppler */
        uint8_t selectCoherentPeakInDopplerDim;

        /*! @brief     1 - 1D: 6x1 azimuth antenna array 2 - 2D: one lower row: 4 antennas, upper row 2 antennas */
        uint8_t angleDimension;

        /*! @brief     Detection matrix format 0 - Linear 32-bit integer, 1 - 16-bit log2 magnitude in Q11 format */
        bool isDetMatrixLogScale;

        /*! @brief   Static Clutter Removal Cfg */
        bool isStaticClutterRemovalEnabled;

        /*! @brief   Range Bias and rx channel gain/phase compensation enable flag */
        bool isRxChGainPhaseCompensationEnabled;

        /*! @brief   Range Bias and rx channel gain/phase compensation configuration */
        DPU_AoasvcProc_compRxChannelBiasCfg compRxChanCfg;

        /*! @brief   Range loop type: 0 - HWA internal loop, 1- Loop controlled by CPU */
        uint16_t aoasvcRangeLoopType;

        /*! @brief  Major motion detection enable flag */
        bool enableMajorMotion;

        /*! @brief  Minor motion detection enable flag */
        bool enableMinorMotion;

        /*! @brief  Number of virtual antenna rows */
        uint16_t numAntRow;

        /*! @brief  Number of virtual antenna columns */
        uint16_t numAntCol;


        float   rangeStep;
        float   azimStart;
        float   azimStep;
        float   elevStart;
        float   elevStep;
        int16_t numAzimVec;
        int16_t numElevVec;
        bool    enableSteeringVectorCorrection;
        bool    enableAngleInterpolation;

    } DPU_AoasvcProc_StaticConfig;

    /**
     * @brief
     *  dopplerProc DPU configuration parameters
     *
     * @details
     *  The structure is used to hold the configuration parameters
     *  for the AOASVC DPU
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_Config_t
    {
        /*! @brief HW resources. */
        DPU_AoasvcProc_HW_Resources hwRes;

        /*! @brief Static configuration. */
        DPU_AoasvcProc_StaticConfig staticCfg;

    } DPU_AoasvcProc_Config;


    /**
     * @brief
     *  DPU processing output parameters
     *
     * @details
     *  The structure is used to hold the output parameters DPU processing
     *
     *  \ingroup DPU_AOASVCPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_AoasvcProc_OutParams_t
    {
        /*! @brief DPU statistics */
        DPU_AoasvcProc_Stats stats;

        /*! @brief  List with virtual antenna elements */
        DPU_AoasvcProc_VirtualAntennaElements *virtAntElemList;

    } DPU_AoasvcProc_OutParams;


    DPU_AoasvcProc_Handle DPU_AoasvcProc_init(DPU_AoasvcProc_InitParams *initCfg, int32_t *errCode);
    int32_t               DPU_AoasvcProc_process(
                      DPU_AoasvcProc_Handle             handle,
                      uint32_t                          numPointsMajor,
                      uint32_t                          numPointsMinor,
                      DPIF_PointCloudCartesianExt      *detObjOut,
                      DPIF_PointCloudRngAzimElevDopInd *detObjIndOut,
                      DPU_AoasvcProc_OutParams         *outParams);
    int32_t DPU_AoasvcProc_deinit(DPU_AoasvcProc_Handle handle);
    int32_t DPU_AoasvcProc_config(DPU_AoasvcProc_Handle handle, DPU_AoasvcProc_Config *cfg);

#if 0
int32_t DPU_AoasvcProcHWA_control
(
   DPU_AoAProcHWA_Handle handle,
   DPU_AoAProcHWA_Cmd cmd,
   void *arg,
   uint32_t argSize
);
#endif


#ifdef __cplusplus
}
#endif

#endif
