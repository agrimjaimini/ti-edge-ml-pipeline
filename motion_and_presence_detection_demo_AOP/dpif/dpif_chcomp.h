/**
 *   @file  dpif_radarcube.h
 *
 *   @brief
 *      Defines the data path radar cube data interface.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2023 Texas Instruments, Inc.
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
#ifndef DPIF_CHCOMP_H
#define DPIF_CHCOMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Range Bias and rx channel gain/phase compensation configuration.
     *
     *
     *  \ingroup    DPU_DOAPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPIF_compRxChannelBiasCfg_t
    {

        /*! @brief  Compensation for range estimation bias in meters */
        float rangeBias;

        /*! @brief  Compensation for Rx channel phase bias in Q20 format.
         *          The order here is like x[tx][rx] where rx order is 0,1,....SYS_COMMON_NUM_RX_CHANNEL-1
         *          and tx order is 0, 1,...,SYS_COMMON_NUM_TX_ANTENNAS-1 */
        cmplx32ReIm_t rxChPhaseComp[SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];

    } DPIF_compRxChannelBiasCfg;

    /**
     * @brief Range Bias and rx channel gain/phase compensation configuration.
     *
     *
     *  \ingroup    DPU_DOAPROC_EXTERNAL_DATA_STRUCTURE
     *
     */
    typedef struct DPIF_compRxChannelBiasFloatCfg_t
    {

        /*! @brief  Compensation for range estimation bias in meters */
        float rangeBias;

        /*! @brief  Compensation for Rx channel phase bias in Q20 format.
         *          The order here is like x[tx][rx] where rx order is 0,1,....SYS_COMMON_NUM_RX_CHANNEL-1
         *          and tx order is 0, 1,...,SYS_COMMON_NUM_TX_ANTENNAS-1 */
        float rxChPhaseComp[2 * SYS_COMMON_NUM_TX_ANTENNAS * SYS_COMMON_NUM_RX_CHANNEL];

    } DPIF_compRxChannelBiasFloatCfg;

#ifdef __cplusplus
}
#endif

#endif /* DPIF_CHCOMP_H */
