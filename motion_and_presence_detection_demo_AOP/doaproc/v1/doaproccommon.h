/**
 *   @file  doaproccommon.h
 *
 *   @brief
 *      Implements Common definition across DoA Proc DPU.
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
#ifndef DOAPROC_COMMON_H
#define DOAPROC_COMMON_H

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <dpedma/v1/dpedma.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief
     *  doaProc DPU EDMA configuration parameters
     *
     * @details
     *  The structure is used to hold the EDMA configuration parameters
     *  for the Doppler Processing DPU
     *
     *  \ingroup DPU_DOAPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_DoaProc_Edma_t
    {
        /*! @brief  EDMA channels for circularly stored data in radar cube
         *  to linearized copy into HWA memory */
        DPEDMA_ChanCfg chunk[2];

    } DPU_DoaProc_Edma;


    /**
     * @brief
     *  doaProc DPU statistics
     *
     * @details
     *  The structure is used to hold the statistics of the DPU
     *
     *  \ingroup DPU_DOAPROC_EXTERNAL_DATA_STRUCTURE
     */
    typedef struct DPU_DoaProc_Stats_t
    {
        /*! @brief total number of DPU processing */
        uint32_t numProcess;

        /*! @brief For HWA version of the DPU: total processing time including EDMA transfers.\n
                   For DSP version of the DPU: total processing time excluding EDMA transfers.*/
        uint32_t processingTime;

        /*! @brief time spent waiting for EDMA transfers. Valid only for DSP version of DPU.*/
        uint32_t waitTime;
    } DPU_DoaProc_Stats;

#ifdef __cplusplus
}
#endif

#endif
