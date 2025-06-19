/**
 *   @file  dpif_anglemat.h
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
#ifndef DPIF_ANGLEMAT_H
#define DPIF_ANGLEMAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @defgroup DPIF_ANGLEMATRIX_FORMAT     DPIF_ANGLEMATRIX_FORMAT
 * @brief    Combination of C structure declaration and Content that uniquely describes the angle mtarix on which Azimuth-Elevation FFT is to be applied
 *
 *
 * # |Declaration                                                           |Content
 *---| ---------------------------------------------------------------------|-----------------------------
 * 1 |cmplx16ImRe_t x[numAntennasAzimuth][numAntennasElevation]| After Doppler FFT, before Angle FFT is applied this is one of the ways to store the Angle Matrix
 * 2 |cmplx16ImRe_t x[numAntennasElevation][numAntennasAzimuth]| After Doppler FFT, before Angle FFT is applied this is one of the ways to store the Angle Matrix
 * @{
 */
#define DPIF_DPIF_ANGLEMATRIX_FORMAT_1 1 /*!<  This format is for Angle Matrix after Doppler FFT and before Angle FFT*/

#define DPIF_DPIF_ANGLEMATRIX_FORMAT_2 2 /*!<  This format is for Angle Matrix after Doppler FFT and before Angle FFT*/

    /** @}*/ /*DPIF_ANGLEMATRIX_FORMAT*/


    /**
     * @brief
     *  Angle Matrix Buffer Interface
     *
     * @details
     *  The structure defines the angle matrix buffer interface, including
     * property, size and data pointer.
     */
    typedef struct DPIF_Anglemat_t
    {
        /*! @brief  Angle Matrix data Format @ref DPIF_ANGLEMATRIX_FORMAT */
        uint32_t datafmt;

        /*! @brief  Angle Matrix buffer size in bytes */
        uint32_t dataSize;

        /*! @brief  Angle Matrix data pointer
                    User could remap this to specific typedef using
                    information in @ref DPIF_ANGLEMATRIX_FORMAT */
        void *data;
    } DPIF_Anglemat;


#ifdef __cplusplus
}
#endif

#endif /* DPIF_ANGLEMAT_H */
