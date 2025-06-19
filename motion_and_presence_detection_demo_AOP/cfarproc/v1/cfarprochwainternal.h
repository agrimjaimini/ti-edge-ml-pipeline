/**
 *   @file  cfarprochwainternal.h
 *
 *   @brief
 *      Implements internal data structure for CFAR Processing with HWA.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018-2021 Texas Instruments, Inc.
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

/** @defgroup CFAR_PROC_DPU_INTERNAL       cfarProc DPU Internal
 */

/**
@defgroup DPU_CFARPROC_INTERNAL_FUNCTION            cfarProc DPU Internal Functions
@ingroup CFAR_PROC_DPU_INTERNAL
@brief
*   The section has a list of all internal API which are not exposed to the external
*   applications.
*/
/**
@defgroup DPU_CFARPROC_INTERNAL_DATA_STRUCTURE      cfarProc DPU Internal Data Structures
@ingroup CFAR_PROC_DPU_INTERNAL
@brief
*   The section has a list of all internal data structures which are used internally
*   by the cfarProc DPU module.
*/
/**
@defgroup DPU_CFARPROC_INTERNAL_DEFINITION          cfarProc DPU Internal Definitions
@ingroup CFAR_PROC_DPU_INTERNAL
@brief
*   The section has a list of all internal definitions which are used internally
*   by the cfarProc DPU.
*/


/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/
#ifndef DPU_CFAR_HWA_H
#define DPU_CFAR_HWA_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* mmWave SDK Include Files */
#include "cfarprochwa.h"
#include <utils/mathutils/mathutils.h>

#ifdef __cplusplus
extern "C"
{
#endif


    /**
     * @brief
     *  CFAR detection output
     *
     * @details
     *  The holds CFAR detections with SNR
     *
     */
    typedef volatile struct DPU_CFARProc_CFARDetList_t
    {
        uint16_t rangeIdx; /*!< Range index */
        uint16_t azimuthIdx; /*!< Azimuth index */
        int16_t  snr; /*!< Signal to noise power ratio in steps of 0.1 dB */
        int16_t  noise; /*!< Noise level in steps of 0.1 dB */
    } DPU_CFARProc_CFARDetList;


    /**
     * @brief
     *  HWA CFAR configuration
     *
     * @details
     *  The structure is used to hold the HWA configuration used for CFAR
     *
     *  \ingroup DPU_CFARPROC_INTERNAL_DATA_STRUCTURE
     */
    typedef struct CFARHwaObj_t
    {
        /*! @brief      number of detected objection from HWA */
        uint16_t numHwaCfarObjs;

        /*! @brief      number of detected objection from HWA */
        uint16_t numHwaCfarScndPassObjs;

        /*! @brief HWA Handle */
        HWA_Handle hwaHandle;

        /*! @brief Hardware resources */
        DPU_CFARProcHWA_HW_Resources res;

        /*! @brief     HWA Processing Done semaphore Handle */
        SemaphoreP_Object hwaDone_semaHandle;

        /*! @brief     EDMA Processing Done semaphore Handle */
        SemaphoreP_Object edmaDoneSemaHandle;

        /*! @brief      CFAR configuration in range direction */
        DPU_CFARProc_CfarCfg cfarCfg;

        /*! @brief      CFAR second pass configuration  */
        DPU_CFARProc_CfarScndPassCfg cfarScndPassCfg;

        /*! @brief      Detection configuration */
        DPU_CFARProc_detectionCfg detectionCfg;

        /*! @brief      Static configuration */
        DPU_CFARProcHWA_StaticConfig staticCfg;

        /*! @brief total number of calls of DPU processing */
        uint32_t numProcess;

        /*! @brief HWA Params save location
         */
        DPU_CFARProcHWA_HwaParamSaveLoc hwaParamsSaveLoc;

        /*! @brief save the value of 1 / AzimuthFFTSize
         */
        float oneOverAzimFftSize;

        /*! @brief save the value of 1 / ElevationFFTSize
         */
        float oneOverElevFftSize;

    } CFARHwaObj;


#ifdef __cplusplus
}
#endif

#endif /* DPU_CFAR_HWA_H */
