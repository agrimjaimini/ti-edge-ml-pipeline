/**
 *   @file  dpedma.c
 *
 *   @brief
 *      EDMA Configuration Utility API implementation.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018 - 2021 Texas Instruments, Inc.
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

#include <kernel/dpl/CacheP.h>
#include <drivers/soc.h>
#include <drivers/hw_include/hw_types.h>
#include <datapath/dpedma/v0/dpedma.h>

/**
 *  @b Description
 *  @n
 *     Utility function for linking EDMA channel with a shadow link paramset
 *
 *  @param[in]  handle          EDMA handle.
 *  @param[in]  chId            EDMA channel id
 *  @param[in]  shadowParamId   EDMA channel shadow id
 *  @param[in]  config          Pointer to EDMA paramset configuration
 *
 *  \ingroup    DPEDMA_INTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 */
int32_t DPEDMA_setup_shadow_link(
    EDMA_Handle       handle,
    uint8_t           paramId,
    uint16_t          shadowParamId,
    EDMACCPaRAMEntry *config)
{
    int32_t          errorCode = DPEDMA_NO_ERROR;
    uint32_t         baseAddr;
    EDMACCPaRAMEntry edmaParam;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        // System_printf("Error: EDMA_getBaseAddr() failed \n");
        goto exit;
    }
    DebugP_assert(baseAddr != 0);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);
    edmaParam = *config;

    EDMASetPaRAM(baseAddr, shadowParamId, &edmaParam);

    EDMALinkChannel(baseAddr, paramId, shadowParamId);

    EDMALinkChannel(baseAddr, shadowParamId, shadowParamId);

exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *     EDMA utility function for sync AB type transfers.
 *     This is similar to DPEDMA_configSyncAB. It just has extra argument
 *     to configure EDMA transfer type, EDMA_SYNC_AB or EDMA_SYNC_A
 *
 *
 *  @param[in]  handle          EDMA handle.
 *  @param[in]  chanCfg         Pointer to datapath EDMA channel configuration
 *  @param[in]  chainingCfg     Pointer to datapath EDMA channel chaining configuration
 *  @param[in]  syncABCfg       Pointer to syncAB type configuration
 *  @param[in]  isEventTriggered  Flag indicates if the channel is event triggered
 *  @param[in]  isIntermediateTransferCompletionEnabled Set to 'true' if intermedate transfer completion
 *                 indication is to be enabled.
 *  @param[in]  isTransferCompletionEnabled Set to 'true' if final transfer completion
 *                              indication is to be enabled.
 *  @param[in]  Edma_EventCallback Transfer completion call back function.
 *  @param[in]  transferCompletionCallbackFxnArg Argument for transfer completion call back function.
 *  @param[in]  transferType EDMA transfer type, (EDMA_SYNC_AB or EDMA_SYNC_A)
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 */
int32_t DPEDMA_configSyncTransfer(
    EDMA_Handle         handle,
    DPEDMA_ChanCfg     *chanCfg,
    DPEDMA_ChainingCfg *chainingCfg,
    DPEDMA_syncABCfg   *syncABCfg,
    bool                isEventTriggered,
    bool                isIntermediateTransferCompletionEnabled,
    bool                isTransferCompletionEnabled,
    Edma_EventCallback  transferCompletionCallbackFxn,
    void               *transferCompletionCallbackFxnArg,
    Edma_IntrObject    *intrObj,
    uint8_t             transferType)
{
    volatile uint32_t baseAddr, regionId;
    uint32_t          dmaCh, tcc, param, chType;
    EDMACCPaRAMEntry  edmaParam;
    int32_t           errorCode = DPEDMA_NO_ERROR;
    int32_t           status    = SystemP_SUCCESS;

    if ((chanCfg == NULL) || (syncABCfg == NULL))
    {
        errorCode = DPEDMA_EINVAL;
        goto exit;
    }

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        // System_printf("Error: EDMA_getBaseAddr() failed \n");
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    dmaCh  = chanCfg->channel;
    param  = chanCfg->channel;
    chType = (uint8_t)EDMA_CHANNEL_TYPE_DMA;

    if (chainingCfg != NULL)
    {
        tcc = chainingCfg->chainingChan;
    }
    else
    {
        tcc = chanCfg->channel;
    }

    status = EDMA_allocDmaChannel(handle, &dmaCh);
    DebugP_assert(status == SystemP_SUCCESS);

    // status = EDMA_allocTcc(handle, &tcc);
    // DebugP_assert(status == SystemP_SUCCESS);

    status = EDMA_allocParam(handle, &param);
    DebugP_assert(status == SystemP_SUCCESS);

    /* Request channel */
    status = EDMAConfigureChannelRegion(baseAddr, regionId, chType, dmaCh, tcc, param, 0);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);
    edmaParam.srcAddr    = (uint32_t)SOC_virtToPhy((void *)syncABCfg->srcAddress);
    edmaParam.destAddr   = (uint32_t)SOC_virtToPhy((void *)syncABCfg->destAddress);
    edmaParam.aCnt       = (uint16_t)syncABCfg->aCount;
    edmaParam.bCnt       = (uint16_t)syncABCfg->bCount;
    edmaParam.cCnt       = (uint16_t)syncABCfg->cCount;
    edmaParam.bCntReload = (uint16_t)0U;
    edmaParam.srcBIdx    = (int16_t)syncABCfg->srcBIdx;
    edmaParam.destBIdx   = (int16_t)syncABCfg->dstBIdx;
    edmaParam.srcCIdx    = (int16_t)syncABCfg->srcCIdx;
    edmaParam.destCIdx   = (int16_t)syncABCfg->dstCIdx;
    edmaParam.linkAddr   = 0xFFFFU;

    /*
     * (BIT-0, BIT-1)  -   Source Address Mode & destination Address Mode is by default Linear
     * (BIT-3)         -   Static Entry is by default normal
     * (BIT-8 - BIT-10)-   FIFO width is by default 8-bit
     * (BIT-11)        -   Early completion is by default false
     */
    if ((uint32_t)transferType == EDMA_SYNC_AB)
    {
        edmaParam.opt |= (EDMA_OPT_SYNCDIM_MASK);
    }

    if (isTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_TCINTEN_MASK);
    }

    if (isIntermediateTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_ITCINTEN_MASK);
    }

    if ((uint32_t)transferType == EDMA_SYNC_AB)
    {
        edmaParam.opt |= (EDMA_OPT_SYNCDIM_MASK);
    }

    /* Chaining channel Cfg. */
    if (chainingCfg != NULL)
    {
        edmaParam.opt |= ((((uint32_t)chainingCfg->chainingChan) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK);
        if (chainingCfg->isFinalChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_TCCHEN_MASK);
        }

        if (chainingCfg->isIntermediateChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_ITCCHEN_MASK);
        }
    }
    EDMASetPaRAM(baseAddr, param, &edmaParam);

    if (isEventTriggered)
    {
        EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_EVENT);
    }

    if (transferCompletionCallbackFxn != NULL)
    {
        /* Register interrupt */
        intrObj->tccNum  = chainingCfg->chainingChan;
        intrObj->cbFxn   = transferCompletionCallbackFxn;
        intrObj->appData = (void *)transferCompletionCallbackFxnArg;
        errorCode        = EDMA_registerIntr(handle, intrObj);
        DebugP_assert(errorCode == SystemP_SUCCESS);
    }

    errorCode = DPEDMA_setup_shadow_link(handle, param, chanCfg->channelShadow, &edmaParam);

exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *      Configure EDMA channel as dummy channel.
 *
 *  @param[in]  handle  EDMA handle
 *  @param[in]  chType  channel type
 *  @param[in]  dmaCh   EDMA Channel id
 *  @param[in]  tcc     TCC id
 *  @param[in]  param   PARAM id
 *  @param[in]  isEventTriggered is event triggered flag
 *
 *  \ingroup    DPU_RANGEPROC_INTERNAL_FUNCTION
 *
 *  @retval
 *      Success     - 0
 *  @retval
 *      Error       - !=0
 */
int32_t DPEDMA_configDummyChannel(EDMA_Handle handle,
                                  uint32_t    chType,
                                  uint32_t   *dmaCh,
                                  uint32_t   *tcc,
                                  uint32_t   *param)
{
    uint32_t         baseAddr, regionId;
    EDMACCPaRAMEntry edmaParam;
    int32_t          status = SystemP_SUCCESS;
    bool             retVal;

    baseAddr = EDMA_getBaseAddr(handle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    status = EDMA_allocDmaChannel(handle, dmaCh);
    DebugP_assert(status == SystemP_SUCCESS);

    // status = EDMA_allocTcc(handle, tcc);
    // DebugP_assert(status == SystemP_SUCCESS);

    status = EDMA_allocParam(handle, param);
    DebugP_assert(status == SystemP_SUCCESS);

    /* Request channel */
    retVal = EDMAConfigureChannelRegion(baseAddr, regionId, chType, *dmaCh, *tcc, *param, 0);
    DebugP_assert(retVal == true);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);

    edmaParam.srcAddr    = (uint32_t)NULL;
    edmaParam.destAddr   = (uint32_t)NULL;
    edmaParam.aCnt       = (uint16_t)1;
    edmaParam.bCnt       = (uint16_t)0;
    edmaParam.cCnt       = (uint16_t)0;
    edmaParam.bCntReload = (uint16_t)0;
    edmaParam.srcBIdx    = (int16_t)0;
    edmaParam.destBIdx   = (int16_t)0;
    edmaParam.srcCIdx    = (int16_t)0;
    edmaParam.destCIdx   = (int16_t)0;
    edmaParam.linkAddr   = (0xFFFFU);
    EDMASetPaRAM(baseAddr, *param, &edmaParam);

    return status;
}

/**
 *  @b Description
 *  @n
 *     EDMA utility function for sync AB type transfers.
 *
 *  @param[in]  handle          EDMA handle.
 *  @param[in]  chanCfg         Pointer to datapath EDMA channel configuration
 *  @param[in]  chainingCfg     Pointer to datapath EDMA channel chaining configuration
 *  @param[in]  syncABCfg       Pointer to syncAB type configuration
 *  @param[in]  isEventTriggered  Flag indicates if the channel is event triggered
 *  @param[in]  isIntermediateTransferCompletionEnabled Set to 'true' if intermedate transfer completion
 *                 indication is to be enabled.
 *  @param[in]  isTransferCompletionEnabled Set to 'true' if final transfer completion
 *                              indication is to be enabled.
 *  @param[in]  Edma_EventCallback Transfer completion call back function.
 *  @param[in]  transferCompletionCallbackFxnArg Argument for transfer completion call back function.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 */
int32_t DPEDMA_configSyncAB(
    EDMA_Handle         handle,
    DPEDMA_ChanCfg     *chanCfg,
    DPEDMA_ChainingCfg *chainingCfg,
    DPEDMA_syncABCfg   *syncABCfg,
    bool                isEventTriggered,
    bool                isIntermediateTransferCompletionEnabled,
    bool                isTransferCompletionEnabled,
    Edma_EventCallback  transferCompletionCallbackFxn,
    void               *transferCompletionCallbackFxnArg,
    Edma_IntrObject    *intrObj)
{
    volatile uint32_t baseAddr, regionId;
    uint32_t          dmaCh, tcc, param, chType;
    EDMACCPaRAMEntry  edmaParam;
    int32_t           errorCode = DPEDMA_NO_ERROR;
    int32_t           status    = SystemP_SUCCESS;

    if ((chanCfg == NULL) || (syncABCfg == NULL))
    {
        errorCode = DPEDMA_EINVAL;
        goto exit;
    }

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        // System_printf("Error: EDMA_getBaseAddr() failed \n");
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    dmaCh  = chanCfg->channel;
    param  = chanCfg->channel;
    chType = (uint8_t)EDMA_CHANNEL_TYPE_DMA;

    if (chainingCfg != NULL)
    {
        tcc = chainingCfg->chainingChan;
    }
    else
    {
        tcc = chanCfg->channel;
    }

    status = EDMA_allocDmaChannel(handle, &dmaCh);
    DebugP_assert(status == SystemP_SUCCESS);

    // status = EDMA_allocTcc(handle, &tcc);
    // DebugP_assert(status == SystemP_SUCCESS);

    status = EDMA_allocParam(handle, &param);
    DebugP_assert(status == SystemP_SUCCESS);

    /* Request channel */
    status = EDMAConfigureChannelRegion(baseAddr, regionId, chType, dmaCh, tcc, param, 0);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);
    edmaParam.srcAddr    = (uint32_t)SOC_virtToPhy((void *)syncABCfg->srcAddress);
    edmaParam.destAddr   = (uint32_t)SOC_virtToPhy((void *)syncABCfg->destAddress);
    edmaParam.aCnt       = (uint16_t)syncABCfg->aCount;
    edmaParam.bCnt       = (uint16_t)syncABCfg->bCount;
    edmaParam.cCnt       = (uint16_t)syncABCfg->cCount;
    edmaParam.bCntReload = (uint16_t)0U;
    edmaParam.srcBIdx    = (int16_t)syncABCfg->srcBIdx;
    edmaParam.destBIdx   = (int16_t)syncABCfg->dstBIdx;
    edmaParam.srcCIdx    = (int16_t)syncABCfg->srcCIdx;
    edmaParam.destCIdx   = (int16_t)syncABCfg->dstCIdx;
    edmaParam.linkAddr   = 0xFFFFU;

    /*
     * (BIT-0, BIT-1)  -   Source Address Mode & destination Address Mode is by default Linear
     * (BIT-3)         -   Static Entry is by default normal
     * (BIT-8 - BIT-10)-   FIFO width is by default 8-bit
     * (BIT-11)        -   Early completion is by default false
     */
    edmaParam.opt |= (EDMA_OPT_SYNCDIM_MASK);

    if (isTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_TCINTEN_MASK);
    }

    if (isIntermediateTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_ITCINTEN_MASK);
    }

    /* Chaining channel Cfg. */
    if (chainingCfg != NULL)
    {
        edmaParam.opt |= ((((uint32_t)chainingCfg->chainingChan) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK);
        if (chainingCfg->isFinalChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_TCCHEN_MASK);
        }

        if (chainingCfg->isIntermediateChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_ITCCHEN_MASK);
        }
    }
    EDMASetPaRAM(baseAddr, param, &edmaParam);

    if (isEventTriggered)
    {
        EDMAEnableTransferRegion(baseAddr, regionId, dmaCh, EDMA_TRIG_MODE_EVENT);
    }

    if (transferCompletionCallbackFxn != NULL)
    {
        /* Register interrupt */
        intrObj->tccNum  = chainingCfg->chainingChan;
        intrObj->cbFxn   = transferCompletionCallbackFxn;
        intrObj->appData = (void *)transferCompletionCallbackFxnArg;
        errorCode        = EDMA_registerIntr(handle, intrObj);
        DebugP_assert(errorCode == SystemP_SUCCESS);
    }

    errorCode = DPEDMA_setup_shadow_link(handle, param, chanCfg->channelShadow, &edmaParam);

exit:
    return (errorCode);
}


/**
 *  @b Description
 *  @n
 *     EDMA utility function for sync A type transfers.
 *     Here single frame means (EDMA) C count is 1.
 *
 *  @param[in]  handle          EDMA handle.
 *  @param[in]  chanCfg         Pointer to datapath EDMA channel configuration
 *  @param[in]  chainingCfg     Pointer to datapath EDMA channel chaining configuration
 *  @param[in]  syncACfg        Pointer to syncA type configuration
 *  @param[in]  isEventTriggered  Flag indicates if the channel is event triggered
 *  @param[in]  isIntermediateTransferInterruptEnabled Set to 'true' if intermediate transfer completion
 *                 indication is to be enabled.
 *  @param[in]  isTransferCompletionEnabled Set to 'true' if final transfer completion
 *                              indication is to be enabled.
 *  @param[in]  Edma_EventCallback Transfer completion call back function.
 *  @param[in]  transferCompletionCallbackFxnArg Argument for transfer completion call back function.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 */
int32_t DPEDMA_configSyncA_singleFrame(
    EDMA_Handle         handle,
    DPEDMA_ChanCfg     *chanCfg,
    DPEDMA_ChainingCfg *chainingCfg,
    DPEDMA_syncACfg    *syncACfg,
    bool                isEventTriggered,
    bool                isIntermediateTransferInterruptEnabled,
    bool                isTransferCompletionEnabled,
    Edma_EventCallback  transferCompletionCallbackFxn,
    void               *transferCompletionCallbackFxnArg,
    Edma_IntrObject    *intrObj)
{
    uint32_t         baseAddr, regionId;
    uint32_t         dmaCh, tcc, param, chType;
    int32_t          status = SystemP_SUCCESS;
    EDMACCPaRAMEntry edmaParam;
    int32_t          errorCode = DPEDMA_NO_ERROR;

    if ((chanCfg == NULL) || (syncACfg == NULL))
    {
        errorCode = DPEDMA_EINVAL;
        goto exit;
    }

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        // System_printf("Error: EDMA_getBaseAddr() failed \n");
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);


    dmaCh  = chanCfg->channel;
    param  = chanCfg->channel;
    chType = (uint8_t)EDMA_CHANNEL_TYPE_DMA;

    if (chainingCfg != NULL)
    {
        tcc = chainingCfg->chainingChan;
    }
    else
    {
        tcc = chanCfg->channel;
    }

    status = EDMA_allocDmaChannel(handle, &dmaCh);
    DebugP_assert(status == SystemP_SUCCESS);

    // status = EDMA_allocTcc(handle, &tcc);
    // DebugP_assert(status == SystemP_SUCCESS);

    status = EDMA_allocParam(handle, &param);
    DebugP_assert(status == SystemP_SUCCESS);

    /* Request channel */
    status = EDMAConfigureChannelRegion(baseAddr, regionId, chType, dmaCh, tcc, param, 0);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);
    edmaParam.srcAddr    = (uint32_t)SOC_virtToPhy((void *)syncACfg->srcAddress);
    edmaParam.destAddr   = (uint32_t)SOC_virtToPhy((void *)syncACfg->destAddress);
    edmaParam.aCnt       = (uint16_t)syncACfg->aCount;
    edmaParam.bCnt       = (uint16_t)syncACfg->bCount;
    edmaParam.cCnt       = (uint16_t)1U;
    edmaParam.bCntReload = (uint16_t)edmaParam.bCnt;
    edmaParam.srcBIdx    = (int16_t)syncACfg->srcBIdx;
    edmaParam.destBIdx   = (int16_t)syncACfg->dstBIdx;
    edmaParam.linkAddr   = 0xFFFFU;
    /*
     * (BIT-0, BIT-1)  -   Source Address Mode & destination Address Mode is by default Linear
     * (BIT-3)         -   Static Entry is by default normal
     * (BIT-8 - BIT-10)-   FIFO width is by default 8-bit
     * (BIT-11)        -   Early completion is by default false
     */
    edmaParam.opt |= (EDMA_SYNC_A);

    if (isTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_TCINTEN_MASK);
    }

    if (isIntermediateTransferInterruptEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_ITCINTEN_MASK);
    }

    /* Chaining Channel Config. */
    if (chainingCfg != NULL)
    {
        edmaParam.opt |= ((((uint32_t)chainingCfg->chainingChan) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK);

        if (chainingCfg->isFinalChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_TCCHEN_MASK);
        }

        if (chainingCfg->isIntermediateChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_ITCCHEN_MASK);
        }
    }

    EDMASetPaRAM(baseAddr, param, &edmaParam);

    if (isEventTriggered)
    {
        EDMAEnableTransferRegion(baseAddr, regionId, chanCfg->channel, EDMA_TRIG_MODE_EVENT);
    }

    if (transferCompletionCallbackFxn != NULL)
    {
        /* Register interrupt */
        intrObj->tccNum  = chainingCfg->chainingChan;
        intrObj->cbFxn   = transferCompletionCallbackFxn;
        intrObj->appData = (void *)transferCompletionCallbackFxnArg;
        errorCode        = EDMA_registerIntr(handle, intrObj);
        DebugP_assert(errorCode == SystemP_SUCCESS);
    }

    errorCode = DPEDMA_setup_shadow_link(handle, param, chanCfg->channelShadow, &edmaParam);

exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *     EDMA utility function for sync A type transfers.
 *
 *  @param[in]  handle          EDMA handle.
 *  @param[in]  chanCfg         Pointer to datapath EDMA channel configuration
 *  @param[in]  chainingCfg     Pointer to datapath EDMA channel chaining configuration
 *  @param[in]  syncACfg        Pointer to syncA type configuration
 *  @param[in]  isEventTriggered  Flag indicates if the channel is event triggered
 *  @param[in]  isIntermediateTransferInterruptEnabled Set to 'true' if intermediate transfer completion
 *                 indication is to be enabled.
 *  @param[in]  isTransferCompletionEnabled Set to 'true' if final transfer completion
 *                              indication is to be enabled.
 *  @param[in]  Edma_EventCallback Transfer completion call back function.
 *  @param[in]  transferCompletionCallbackFxnArg Argument for transfer completion call back function.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 */
int32_t DPEDMA_configSyncA(
    EDMA_Handle         handle,
    DPEDMA_ChanCfg     *chanCfg,
    DPEDMA_ChainingCfg *chainingCfg,
    DPEDMA_syncACfg    *syncACfg,
    bool                isEventTriggered,
    bool                isIntermediateTransferInterruptEnabled,
    bool                isTransferCompletionEnabled,
    Edma_EventCallback  transferCompletionCallbackFxn,
    void               *transferCompletionCallbackFxnArg,
    Edma_IntrObject    *intrObj)
{
    uint32_t         baseAddr, regionId;
    EDMACCPaRAMEntry edmaParam;
    int32_t          errorCode = DPEDMA_NO_ERROR;

    if ((chanCfg == NULL) || (syncACfg == NULL))
    {
        errorCode = DPEDMA_EINVAL;
        goto exit;
    }

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        // System_printf("Error: EDMA_getBaseAddr() failed \n");
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    /* Program Param Set */
    EDMACCPaRAMEntry_init(&edmaParam);
    edmaParam.srcAddr    = (uint32_t)SOC_virtToPhy((void *)syncACfg->srcAddress);
    edmaParam.destAddr   = (uint32_t)SOC_virtToPhy((void *)syncACfg->destAddress);
    edmaParam.aCnt       = (uint16_t)syncACfg->aCount;
    edmaParam.bCnt       = (uint16_t)syncACfg->bCount;
    edmaParam.cCnt       = (uint16_t)syncACfg->cCount;
    edmaParam.srcCIdx    = (uint16_t)syncACfg->srcCIdx;
    edmaParam.destCIdx   = (uint16_t)syncACfg->dstCIdx;
    edmaParam.bCntReload = (uint16_t)edmaParam.bCnt;
    edmaParam.srcBIdx    = (int16_t)syncACfg->srcBIdx;
    edmaParam.destBIdx   = (int16_t)syncACfg->dstBIdx;
    edmaParam.linkAddr   = 0xFFFFU;
    /*
     * (BIT-0, BIT-1)  -   Source Address Mode & destination Address Mode is by default Linear
     * (BIT-3)         -   Static Entry is by default normal
     * (BIT-8 - BIT-10)-   FIFO width is by default 8-bit
     * (BIT-11)        -   Early completion is by default false
     */
    edmaParam.opt |= (EDMA_SYNC_A);

    if (isTransferCompletionEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_TCINTEN_MASK);
    }

    if (isIntermediateTransferInterruptEnabled)
    {
        edmaParam.opt |= (EDMA_OPT_ITCINTEN_MASK);
    }

    /* Chaining Channel Config. */
    if (chainingCfg != NULL)
    {
        edmaParam.opt |= ((((uint32_t)chainingCfg->chainingChan) << EDMA_OPT_TCC_SHIFT) & EDMA_OPT_TCC_MASK);

        if (chainingCfg->isFinalChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_TCCHEN_MASK);
        }

        if (chainingCfg->isIntermediateChainingEnabled)
        {
            edmaParam.opt |= (EDMA_TPCC_OPT_ITCCHEN_MASK);
        }
    }

    EDMASetPaRAM(baseAddr, chanCfg->paramId, &edmaParam);

    if (isEventTriggered)
    {
        EDMAEnableTransferRegion(baseAddr, regionId, chanCfg->channel, EDMA_TRIG_MODE_EVENT);
    }

    if (transferCompletionCallbackFxn != NULL)
    {
        /* Register interrupt */
        intrObj->tccNum  = chainingCfg->chainingChan;
        intrObj->cbFxn   = transferCompletionCallbackFxn;
        intrObj->appData = (void *)transferCompletionCallbackFxnArg;
        errorCode        = EDMA_registerIntr(handle, intrObj);
        DebugP_assert(errorCode == SystemP_SUCCESS);
    }

    errorCode = DPEDMA_setup_shadow_link(handle, chanCfg->paramId, chanCfg->shadowPramId, &edmaParam);

exit:
    return (errorCode);
}


/**
 *  @b Description
 *  @n
 *    Utility function that reconfigures source and destination addresses of a given channel Id
 *    (whose param Id is assumed to be already configured to be same as channel Id)
 *    and then starts a transfer on that channel.
 *
 *  @param[in]  handle         EDMA handle.
 *  @param[in]  srcAddress     Source Address. If NULL, does not update.
 *  @param[in]  destAddress    Destination Address. If NULL, does not update.
 *  @param[in]  channel        EDMA channel Id.
 *  @param[in]  triggerEnabled =1: trigger EDMA, =0: does not trigger EDMA
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 *
 */
int32_t DPEDMA_updateAddressAndTrigger(EDMA_Handle handle,
                                       uint32_t    srcAddress,
                                       uint32_t    destAddress,
                                       uint8_t     paramId,
                                       uint8_t     triggerEnabled)
{
    int32_t  errorCode = DPEDMA_NO_ERROR;
    uint32_t baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    if (srcAddress != NULL)
    {
        EDMADmaSetPaRAMEntry(baseAddr, paramId, EDMACC_PARAM_ENTRY_SRC, (uint32_t)SOC_virtToPhy((void *)srcAddress));
    }

    if (destAddress != NULL)
    {
        EDMADmaSetPaRAMEntry(baseAddr, paramId, EDMACC_PARAM_ENTRY_DST, (uint32_t)SOC_virtToPhy((void *)destAddress));
    }

    if (triggerEnabled)
    {
        regionId = EDMA_getRegionId(handle);

        if (!EDMAEnableTransferRegion(baseAddr, regionId, paramId, EDMA_TRIG_MODE_MANUAL))
        {
            errorCode = DPEDMA_EDMA_TRIGGER_FAIL;
            goto exit;
        }
    }

exit:
    return (errorCode);
}

/**
 *  @b Description
 *  @n
 *    Utility function that reconfigures BcntAcnt field of a given channel Id
 *
 *  @param[in]  handle         EDMA handle.
 *  @param[in]  chanId         EDMA channel Id.
 *  @param[in]  BcntAcnt       BcntAcnt value.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 *
 */
int32_t DPEDMA_setBcntAcnt(EDMA_Handle handle,
                           uint32_t    chanId,
                           uint32_t    BcntAcnt)
{
    int32_t  errorCode = DPEDMA_NO_ERROR;
    uint32_t baseAddr;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    EDMADmaSetPaRAMEntry(baseAddr, chanId, EDMACC_PARAM_ENTRY_ACNT_BCNT, (uint32_t)BcntAcnt);
exit:
    return errorCode;
}

/**
 *  @b Description
 *  @n
 *    Utility function that reconfigures source addresses of a given channel Id
 *
 *  @param[in]  handle         EDMA handle.
 *  @param[in]  chanId         EDMA channel Id.
 *  @param[in]  addressVal     Source Address.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 *
 */
int32_t DPEDMA_setSourceAddress(EDMA_Handle handle,
                                uint32_t    chanId,
                                uint32_t    addressVal)
{
    int32_t  errorCode = DPEDMA_NO_ERROR;
    uint32_t baseAddr;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    EDMADmaSetPaRAMEntry(baseAddr, chanId, EDMACC_PARAM_ENTRY_SRC, (uint32_t)addressVal);
exit:
    return errorCode;
}

/**
 *  @b Description
 *  @n
 *    Utility function that reconfigures destination addresses of a given channel Id
 *
 *  @param[in]  handle         EDMA handle.
 *  @param[in]  chanId         EDMA channel Id.
 *  @param[in]  addressVal     Source Address.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 *
 */
int32_t DPEDMA_setDestinationAddress(EDMA_Handle handle,
                                     uint32_t    chanId,
                                     uint32_t    addressVal)
{
    int32_t  errorCode = DPEDMA_NO_ERROR;
    uint32_t baseAddr;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    EDMADmaSetPaRAMEntry(baseAddr, chanId, EDMACC_PARAM_ENTRY_DST, (uint32_t)addressVal);
exit:
    return errorCode;
}

/**
 *  @b Description
 *  @n
 *    Utility function that reconfigures CCOUNT of a given channel Id
 *
 *  @param[in]  handle         EDMA handle.
 *  @param[in]  chanId         EDMA channel Id.
 *  @param[in]  cCount         CCOUNT.
 *
 *  \ingroup    DPEDMA_EXTERNAL_FUNCTION
 *
 *  @retval
 *      EDMA driver error code, see "EDMA_ERROR_CODES" in EDMA API.
 *
 */
int32_t DPEDMA_setCcnt(EDMA_Handle handle,
                       uint32_t    chanId,
                       uint32_t    cCount)
{
    int32_t  errorCode = DPEDMA_NO_ERROR;
    uint32_t baseAddr;
    uint32_t paramEntryVal;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        errorCode = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    paramEntryVal = EDMADmaGetPaRAMEntry(baseAddr, chanId, EDMACC_PARAM_ENTRY_CCNT);
    paramEntryVal &= 0xFFFF0000;
    paramEntryVal |= cCount & 0x0000FFFF;
    EDMADmaSetPaRAMEntry(baseAddr, chanId, EDMACC_PARAM_ENTRY_CCNT, paramEntryVal);
exit:
    return errorCode;
}


int32_t DPEDMA_startTransfer(EDMA_Handle handle, uint32_t chanId)
{
    bool     edmaStatus;
    int32_t  retVal = SystemP_SUCCESS;
    uint32_t baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        retVal = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    edmaStatus = EDMAEnableTransferRegion(baseAddr, regionId, chanId, EDMA_TRIG_MODE_MANUAL);

    if (edmaStatus != true)
    {
        retVal = DPEDMA_EINVAL_EDMAADDR;
        goto exit;
    }

exit:
    return retVal;
}


/**
 *  @b Description
 *  @n
 *    Utility function that allocated DMA resources
 *
 */
void DPEDMA_allocateEDMAChannel(EDMA_Handle handle,
                                uint32_t   *dmaCh,
                                uint32_t   *tcc,
                                uint32_t   *param)
{
    int32_t      testStatus = SystemP_SUCCESS;
    uint32_t     baseAddr, regionId;
    EDMA_Config *config;
    EDMA_Object *object;

    config = (EDMA_Config *)handle;
    object = config->object;

    if ((object->allocResource.dmaCh[*dmaCh / 32] & (1U << *dmaCh % 32)) != (1U << *dmaCh % 32))
    {
        testStatus = EDMA_allocDmaChannel(handle, dmaCh);
        DebugP_assert(testStatus == SystemP_SUCCESS);

        // testStatus = EDMA_allocTcc(handle, tcc);
        // DebugP_assert(testStatus == SystemP_SUCCESS);

        testStatus = EDMA_allocParam(handle, param);
        DebugP_assert(testStatus == SystemP_SUCCESS);

        baseAddr = EDMA_getBaseAddr(handle);
        DebugP_assert(baseAddr != 0);

        regionId = EDMA_getRegionId(handle);
        DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

        /* Request channel */
        EDMAConfigureChannelRegion(baseAddr, regionId, EDMA_CHANNEL_TYPE_DMA, *dmaCh, *tcc, *param, 0);
    }

    return;
}

/**
 *  @b Description
 *  @n
 *    Utility function that frees DMA resource
 *
 */
void DPEDMA_freeEDMAChannel(EDMA_Handle handle,
                            uint32_t   *dmaCh,
                            uint32_t   *tcc,
                            uint32_t   *param,
                            uint32_t   *shadowParam)
{
    int32_t  testStatus = SystemP_SUCCESS;
    uint32_t baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(handle);
    DebugP_assert(baseAddr != 0);

    regionId = EDMA_getRegionId(handle);
    DebugP_assert(regionId < SOC_EDMA_NUM_REGIONS);

    EDMAFreeChannelRegion(baseAddr, regionId, EDMA_CHANNEL_TYPE_DMA, *dmaCh, EDMA_TRIG_MODE_MANUAL, *tcc, 0);

    /* Free the EDMA resources managed by driver. */
    testStatus = EDMA_freeDmaChannel(handle, dmaCh);
    DebugP_assert(testStatus == SystemP_SUCCESS);
    *dmaCh = 0;

    testStatus = EDMA_freeTcc(handle, tcc);
    DebugP_assert(testStatus == SystemP_SUCCESS);
    *tcc = 0;

    testStatus = EDMA_freeParam(handle, param);
    DebugP_assert(testStatus == SystemP_SUCCESS);
    *param = 0;

    testStatus = EDMA_freeParam(handle, shadowParam);
    DebugP_assert(testStatus == SystemP_SUCCESS);
    *shadowParam = 0;

    return;
}

/**
 *  @b Description
 *  @n
 *    Utility function to perform manual trigger of EDMA
 *
 */
int32_t DPEDMA_edmaStartTransferManualTrigger(EDMA_Handle handle,
                                              uint32_t    channel)
{

    int32_t  retVal = 0;
    uint32_t baseAddr, regionId;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        retVal = -1;
        goto exit;
    }

    regionId = EDMA_getRegionId(handle);
    if (regionId > SOC_EDMA_NUM_REGIONS)
    {
        retVal = -1;
        goto exit;
    }

    if (!EDMAEnableTransferRegion(baseAddr, regionId, channel, EDMA_TRIG_MODE_MANUAL))
    {
        retVal = -1;
    }

exit:
    return retVal;
}

/**
 *  @b Description
 *  @n
 *    ToDo Add this to EDMA driver
 *
 */
int32_t DPEDMA_linkParamSets(EDMA_Handle handle, uint16_t fromParamId, uint16_t toParamId)
{
    int32_t  retVal = SystemP_SUCCESS;
    uint32_t baseAddr;

    baseAddr = EDMA_getBaseAddr(handle);
    if (baseAddr == 0)
    {
        retVal = SystemP_FAILURE;
        goto exit;
    }

    HW_WR_FIELD32(baseAddr + EDMA_TPCC_LNK((uint32_t)fromParamId),
                  EDMA_TPCC_LNK_LINK,
                  baseAddr + EDMA_TPCC_OPT((uint32_t)toParamId));

exit:
    return (retVal);
}


/**
 *  @b Description
 *  @n
 *    Translates local core address to global address
 *
 */
uint32_t DPEDMA_local2Global(uint32_t addr)
{
    // If not really local, do not touch
    if ((addr < 0x00400000) || (addr > 0x0047ffff))
    {
        return addr; // do not touch
    }
    else
    {
        /* Compute the global address. */
        return (0x22000000 | addr);
    }
}
