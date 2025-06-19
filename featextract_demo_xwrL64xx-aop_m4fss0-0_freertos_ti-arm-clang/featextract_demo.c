/*
 *  Copyright (C) 2018-2021 Texas Instruments Incorporated
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

/* Standard Include Files. */
#include <stdio.h>

/* MCU Plus Include Files. */
#include <kernel/dpl/DebugP.h>
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"
#include "ti_board_open_close.h"
#include <kernel/dpl/HeapP.h>
#include <kernel/dpl/CycleCounterP.h>

/* Feature extraction library Include Files*/
#include <alg/featExtract/spectrumBased/featExtract.h>

#define BIN_FILENAME_MAX_LEN 256

/* User defined heap memory and handle */
#define MY_HEAP_MEM_SIZE  (16*1024u)
static uint8_t gMyHeapMem[MY_HEAP_MEM_SIZE] __attribute__((aligned(HeapP_BYTE_ALIGNMENT)));

/* Feature extraction variables */
HeapP_Object gFeatExtractHeapObj;
static bool internalMalloc = false;
static bool printForDebug = false;
static float fltEpsilon = 0.001f;

/* Clock and time measurement APIs variables */
static uint32_t startCycle, cycleCount;

void *featExtract_malloc(uint32_t sizeInBytes)
{
    return HeapP_alloc(&gFeatExtractHeapObj, sizeInBytes);
}
void featExtract_free(void *pFree, uint32_t sizeInBytes)
{
    HeapP_free(&gFeatExtractHeapObj, pFree);
}

void featextract_demo_main(void *args)
{
    /* Open drivers to open the UART driver for console */
    Drivers_open();
    Board_driversOpen();

    DebugP_log("Feature Extraction Test Started!\r\n");

    /* create heap */
    HeapP_construct(&gFeatExtractHeapObj, gMyHeapMem, MY_HEAP_MEM_SIZE);
    DebugP_log("Benchmark data generation started: heap free size = %d bytes\r\n",
            (uint32_t)HeapP_getFreeHeapSize(&gFeatExtractHeapObj)
            );

    FeatExtract_moduleConfig config;
    FeatExtract_featOutput featOut;

    /* Number of features */
    uint32_t numFeatures;
    numFeatures = (uint32_t)(sizeof(FeatExtract_featOutput) / sizeof(float));
    DebugP_log("Number of available features: %d\n", numFeatures);

    void *handle;
	int32_t errCode;
    float *spec;

	FILE *fpCfg, *fpSpec, *fpFeatRef, *fpFeat, *fpCycle;
	char fileNameCfg[BIN_FILENAME_MAX_LEN];
    char fileNameSpec[BIN_FILENAME_MAX_LEN];
    char fileNameFeatRef[BIN_FILENAME_MAX_LEN];
    char fileNameFeat[BIN_FILENAME_MAX_LEN];
    char fileNameCycle[BIN_FILENAME_MAX_LEN];
    sprintf(fileNameCfg, "../../../testBench/fileCfgIn.bin");
    sprintf(fileNameSpec, "../../../testBench/fileSpecIn.bin");
    sprintf(fileNameFeatRef, "../../../testBench/fileFeatRef.bin");
    sprintf(fileNameFeat, "../../../testBench/fileFeatOut.bin");
    sprintf(fileNameCycle, "../../../testBench/fileCycleOut.bin");

    /* Open the input file */
    uint32_t i, n, numTests;
    fpCfg = fopen(fileNameCfg, "rb");
    DebugP_assert(fpCfg!=NULL);

    /* Read the number of tests from the file */
    fread(&numTests, sizeof(uint32_t), 1, fpCfg);
    DebugP_log("numTests: %d\n", numTests);
    
    /* Open the spectrum file */
    fpSpec = fopen(fileNameSpec, "rb");
    DebugP_assert(fpSpec!=NULL);

    /* Create the output features file */
    fpFeat = fopen(fileNameFeat, "wb");
    DebugP_assert(fpFeat!=NULL);

    /* Create the cycle count file */
    fpCycle = fopen(fileNameCycle, "wb");
    DebugP_assert(fpCycle!=NULL);

    /* Enable and reset CPU cycle counter */
    CycleCounterP_reset();

    /* Read the parameters from a file */
    for (i = 0; i < numTests; i++) {
        fread(&config.numFreqBins, sizeof(uint32_t), 1, fpCfg);
        fread(&config.freqStep, sizeof(float), 1, fpCfg);
        fread(&config.pwrThre, sizeof(float), 2, fpCfg);
        fread(&config.specShiftMode, sizeof(uint32_t), 1, fpCfg);

        /* (Optional) print the config parameters */
        if (printForDebug == true) {
            DebugP_log("numFreqBins: %d\n", config.numFreqBins);
            DebugP_log("freqStep: %f\n", config.freqStep);
            DebugP_log("pwrThre: [%f, %f]\n", config.pwrThre.low, config.pwrThre.up);
            DebugP_log("specShiftMode: %d\n", config.specShiftMode);
        }

        /* Allocate internal memory or not */
        if (internalMalloc == true) {
            config.scratchBuffer = NULL;
            config.scratchBufferSizeInBytes = 0;
        }
        else {
            config.scratchBuffer = (float*)HeapP_alloc(&gFeatExtractHeapObj, (config.numFreqBins + 1) * sizeof(float));
            config.scratchBufferSizeInBytes = (config.numFreqBins + 1) * sizeof(float);
            DebugP_assert(config.scratchBuffer!=NULL);       
        }

        /* Call the create function */
        handle = featExtract_create(&config, &errCode);

        /* Read the spectrum file */
        spec = (float*)HeapP_alloc(&gFeatExtractHeapObj, config.numFreqBins * sizeof(float));
        fread(spec, config.numFreqBins * sizeof(float), 1, fpSpec);

        /* (Optional) print the spectrum */
        if (printForDebug == true) {
            DebugP_log("spec (original): ");
            for (n = 0; n < config.numFreqBins; n++) {
                DebugP_log("%f ", spec[n]);
            }
            DebugP_log("\n");
        }

        /* Call the compute function */
        startCycle = CycleCounterP_getCount32(); /* get CPU cycle count */
        featExtract_compute(handle, spec, &featOut);
        cycleCount = CycleCounterP_getCount32() - startCycle; /* get CPU cycle count and calculate diff, we dont expect any overflow for this short duration */

        /* (Optional) print the cycle */
        if (printForDebug == true) {
            DebugP_log("Measured processing time (featExtract_compute) = CPU cycles = %d !!!\r\n", cycleCount);
        }

        /* Write the cycle */
        fwrite(&cycleCount, sizeof(uint32_t), 1, fpCycle);

        /* (Optional) print the spectrum */
        if (printForDebug == true) {
            DebugP_log("spec (after shift): ");
            for (n = 0; n < config.numFreqBins; n++) {
                DebugP_log("%f ", spec[n]);
            }
            DebugP_log("\n");
        }

        /* Write the output */
        fwrite(&featOut, sizeof(FeatExtract_featOutput), 1, fpFeat);

        /* (Optional) print the features */
        if (printForDebug == true) {
            DebugP_log("fLow, fUp: [%f, %f]\n", featOut.fLow, featOut.fUp);
            DebugP_log("meanFreq, medFreq: [%f, %f]\n", featOut.meanFreq, featOut.medFreq);
            DebugP_log("bwPwr, sEntropy: [%f, %f]\n", featOut.bwPwr, featOut.sEntropy);
        }

        /* Call the delete function */           
        featExtract_delete(handle);

        /* Free the memory */
        if (internalMalloc == false)
            HeapP_free(&gFeatExtractHeapObj, config.scratchBuffer);
        HeapP_free(&gFeatExtractHeapObj, spec);

        /* Status (every 10th test) */
        if ((i%10) == 0) {
            DebugP_log("Test %d/%d is processed...\n", i+1, numTests);
        }
    }

    fclose(fpCfg);
    fclose(fpSpec);
    fclose(fpFeat);
    fclose(fpCycle);

    DebugP_log("Benchmark data generation ended: heap free size = %d bytes\r\n",
            (uint32_t)HeapP_getFreeHeapSize(&gFeatExtractHeapObj)
            );
    HeapP_destruct(&gFeatExtractHeapObj);

    DebugP_log("Benchmark validation started!\r\n");

    /* Open the reference and output features file */
    fpFeat = fopen(fileNameFeat, "rb");
    DebugP_assert(fpFeat!=NULL);
    fpFeatRef = fopen(fileNameFeatRef, "rb");
    DebugP_assert(fpFeatRef!=NULL);

    float feat, featRef, featError;
    int32_t errCount = 0, testCount = 0;
    for (i = 0; i < numTests*numFeatures; i++) {
        fread(&feat, sizeof(float), 1, fpFeat);
        fread(&featRef, sizeof(float), 1, fpFeatRef);
        featError = feat - featRef;
        if (fabs(featError) > fltEpsilon) {
            errCount++;
        }
        testCount++;
    }

    fclose(fpFeat);
    fclose(fpFeatRef);
    
    DebugP_log("Total features compared: %d!!\r\n", testCount);
    DebugP_log("Total features failed: %d!!\r\n", errCount);
    DebugP_log("Overal test performance: %%%.2f!!\r\n", (float)(testCount-errCount) / (float)testCount * 100.0f);

    DebugP_log("Benchmark validation ended!\r\n");

    DebugP_log("Feature Extraction Test Ended!\r\n");

    Board_driversClose();
    Drivers_close();
}

