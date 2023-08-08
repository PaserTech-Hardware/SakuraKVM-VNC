#include <vencoder.h>
#include <memoryAdapter.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "logging.h"
#include "cedar_h264.h"

h264_func_t g_h264_func;
VideoEncoder* g_pVideoEnc = NULL;
struct ScMemOpsS * g_videoEncMemOps = NULL;
unsigned int g_max_nSizeY = 0;
unsigned int g_max_nSizeC = 0;
unsigned int g_fps;
long long g_pts = 0;

void init_mb_mode(VencMBModeCtrl *pMBMode, unsigned width, unsigned height)
{
    unsigned int mb_num;
    unsigned int j;

    mb_num = (ALIGN_XXB(16, width) >> 4)
                * (ALIGN_XXB(16, height) >> 4);
    pMBMode->p_info = malloc(sizeof(VencMBModeCtrlInfo) * mb_num);
    pMBMode->mode_ctrl_en = 1;

    for (j = 0; j < mb_num / 2; j++)
    {
        pMBMode->p_info[j].mb_en = 1;
        pMBMode->p_info[j].mb_skip_flag = 0;
        pMBMode->p_info[j].mb_qp = 22;
    }
    for (; j < mb_num; j++)
    {
        pMBMode->p_info[j].mb_en = 1;
        pMBMode->p_info[j].mb_skip_flag = 0;
        pMBMode->p_info[j].mb_qp = 32;
    }
}

void init_mb_info(VencMBInfo *MBInfo, unsigned width, unsigned height)
{
    MBInfo->num_mb = (ALIGN_XXB(16, width) *
                        ALIGN_XXB(16, height)) >> 8;
    MBInfo->p_para = (VencMBInfoPara *)malloc(sizeof(VencMBInfoPara) * MBInfo->num_mb);
    if(MBInfo->p_para == NULL)
    {
        LOGE("malloc MBInfo->p_para error\n");
        return;
    }
    LOGD("mb_num:%d, mb_info_queue_addr:%p\n", MBInfo->num_mb, MBInfo->p_para);
}


void init_fix_qp(VencH264FixQP *fixQP)
{
    fixQP->bEnable = 1;
    fixQP->nIQp = 35;
    fixQP->nPQp = 35;
}

void init_super_frame_cfg(VencSuperFrameConfig *sSuperFrameCfg)
{
    sSuperFrameCfg->eSuperFrameMode = VENC_SUPERFRAME_NONE;
    sSuperFrameCfg->nMaxIFrameBits = 30000*8;
    sSuperFrameCfg->nMaxPFrameBits = 15000*8;
}

void init_svc_skip(VencH264SVCSkip *SVCSkip)
{
    SVCSkip->nTemporalSVC = T_LAYER_4;
    switch(SVCSkip->nTemporalSVC)
    {
        case T_LAYER_4:
            SVCSkip->nSkipFrame = SKIP_8;
            break;
        case T_LAYER_3:
            SVCSkip->nSkipFrame = SKIP_4;
            break;
        case T_LAYER_2:
            SVCSkip->nSkipFrame = SKIP_2;
            break;
        default:
            SVCSkip->nSkipFrame = NO_SKIP;
            break;
    }
}

void init_aspect_ratio(VencH264AspectRatio *sAspectRatio)
{
    sAspectRatio->aspect_ratio_idc = 255;
    sAspectRatio->sar_width = 4;
    sAspectRatio->sar_height = 3;
}

void init_video_signal(VencH264VideoSignal *sVideoSignal)
{
    sVideoSignal->video_format = 5;
    sVideoSignal->src_colour_primaries = 0;
    sVideoSignal->dst_colour_primaries = 1;
}

void init_intra_refresh(VencCyclicIntraRefresh *sIntraRefresh)
{
    sIntraRefresh->bEnable = 1;
    sIntraRefresh->nBlockNumber = 10;
}

void init_roi(VencROIConfig *sRoiConfig)
{
    sRoiConfig[0].bEnable = 0;
    sRoiConfig[0].index = 0;
    sRoiConfig[0].nQPoffset = 10;
    sRoiConfig[0].sRect.nLeft = 0;
    sRoiConfig[0].sRect.nTop = 0;
    sRoiConfig[0].sRect.nWidth = 1280;
    sRoiConfig[0].sRect.nHeight = 320;

    sRoiConfig[1].bEnable = 0;
    sRoiConfig[1].index = 1;
    sRoiConfig[1].nQPoffset = 10;
    sRoiConfig[1].sRect.nLeft = 320;
    sRoiConfig[1].sRect.nTop = 180;
    sRoiConfig[1].sRect.nWidth = 320;
    sRoiConfig[1].sRect.nHeight = 180;

    sRoiConfig[2].bEnable = 0;
    sRoiConfig[2].index = 2;
    sRoiConfig[2].nQPoffset = 10;
    sRoiConfig[2].sRect.nLeft = 320;
    sRoiConfig[2].sRect.nTop = 180;
    sRoiConfig[2].sRect.nWidth = 320;
    sRoiConfig[2].sRect.nHeight = 180;

    sRoiConfig[3].bEnable = 0;
    sRoiConfig[3].index = 3;
    sRoiConfig[3].nQPoffset = 10;
    sRoiConfig[3].sRect.nLeft = 320;
    sRoiConfig[3].sRect.nTop = 180;
    sRoiConfig[3].sRect.nWidth = 320;
    sRoiConfig[3].sRect.nHeight = 180;
}

void init_alter_frame_rate_info(VencAlterFrameRateInfo *pAlterFrameRateInfo)
{
    memset(pAlterFrameRateInfo, 0 , sizeof(VencAlterFrameRateInfo));
    pAlterFrameRateInfo->bEnable = 0;
    pAlterFrameRateInfo->bUseUserSetRoiInfo = 1;
    pAlterFrameRateInfo->sRoiBgFrameRate.nSrcFrameRate = 25;
    pAlterFrameRateInfo->sRoiBgFrameRate.nDstFrameRate = 5;

    pAlterFrameRateInfo->roi_param[0].bEnable = 0;
    pAlterFrameRateInfo->roi_param[0].index = 0;
    pAlterFrameRateInfo->roi_param[0].nQPoffset = 10;
    pAlterFrameRateInfo->roi_param[0].roi_abs_flag = 1;
    pAlterFrameRateInfo->roi_param[0].sRect.nLeft = 0;
    pAlterFrameRateInfo->roi_param[0].sRect.nTop = 0;
    pAlterFrameRateInfo->roi_param[0].sRect.nWidth = 320;
    pAlterFrameRateInfo->roi_param[0].sRect.nHeight = 320;

    pAlterFrameRateInfo->roi_param[1].bEnable = 0;
    pAlterFrameRateInfo->roi_param[1].index = 0;
    pAlterFrameRateInfo->roi_param[1].nQPoffset = 10;
    pAlterFrameRateInfo->roi_param[1].roi_abs_flag = 1;
    pAlterFrameRateInfo->roi_param[1].sRect.nLeft = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nTop = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nWidth = 320;
    pAlterFrameRateInfo->roi_param[1].sRect.nHeight = 320;
}

void init_enc_proc_info(VeProcSet *ve_proc_set)
{
    ve_proc_set->bProcEnable = 1;
    ve_proc_set->nProcFreq = 3;
}

int initH264Func(h264_func_t *h264_func, unsigned int width, unsigned int height, unsigned int fps)
{
    memset(h264_func, 0, sizeof(h264_func_t));

    //init h264Param
    h264_func->h264Param.bEntropyCodingCABAC = 1;
    h264_func->h264Param.nBitrate = 5*1024*1024;
    h264_func->h264Param.nFramerate = fps;
    h264_func->h264Param.nCodingMode = VENC_FRAME_CODING;
    h264_func->h264Param.nMaxKeyInterval = 15;
    h264_func->h264Param.sProfileLevel.nProfile = VENC_H264ProfileHigh;
    h264_func->h264Param.sProfileLevel.nLevel = VENC_H264Level51;
    h264_func->h264Param.sQPRange.nMinqp = 26;
    h264_func->h264Param.sQPRange.nMaxqp = 51;
    h264_func->h264Param.bLongRefEnable = 0;
    h264_func->h264Param.nLongRefPoc = 0;

#if 1
    h264_func->sH264Smart.img_bin_en = 1;
    h264_func->sH264Smart.img_bin_th = 27;
    h264_func->sH264Smart.shift_bits = 2;
    h264_func->sH264Smart.smart_fun_en = 1;
#endif

    //init VencMBModeCtrl
    init_mb_mode(&h264_func->h264MBMode, width, height);

    //init VencMBInfo
    init_mb_info(&h264_func->MBInfo, width, height);

    //init VencH264FixQP
    init_fix_qp(&h264_func->fixQP);

    //init VencSuperFrameConfig
    init_super_frame_cfg(&h264_func->sSuperFrameCfg);

    //init VencH264SVCSkip
    init_svc_skip(&h264_func->SVCSkip);

    //init VencH264AspectRatio
    init_aspect_ratio(&h264_func->sAspectRatio);

    //init VencH264AspectRatio
    init_video_signal(&h264_func->sVideoSignal);

    //init CyclicIntraRefresh
    init_intra_refresh(&h264_func->sIntraRefresh);

    //init VencROIConfig
    init_roi(h264_func->sRoiConfig);

    //init proc info
    init_enc_proc_info(&h264_func->sVeProcInfo);

    return 0;
}

void cedar_h264mode_init(VideoEncoder *pVideoEnc, unsigned int width, unsigned int height, unsigned int fps)
{
    unsigned int vbv_size = 12*1024*1024;

    initH264Func(&g_h264_func, width, height, fps);

    VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264Param, &g_h264_func.h264Param);
    VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetVbvSize, &vbv_size);
    //VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264FixQP, &g_h264_func.fixQP);

    //VideoEncSetParameter(pVideoEnc, VENC_IndexParamSetOverlay, &g_h264_func.sOverlayInfo);

#ifdef GET_MB_INFO
    VideoEncSetParameter(pVideoEnc, VENC_IndexParamMBInfoOutput, &g_h264_func.MBInfo);
#endif
}

void releaseMb(h264_func_t *h264_func)
{
    VencMBInfo *pMBInfo;
    VencMBModeCtrl *pMBMode;

    pMBInfo = &h264_func->MBInfo;
    pMBMode = &h264_func->h264MBMode;

    if(pMBInfo->p_para)
        free(pMBInfo->p_para);
    if(pMBMode->p_info)
        free(pMBMode->p_info);
}

int cedar_hardware_init(unsigned int width, unsigned int height, unsigned int fps)
{
    VencBaseConfig baseConfig;
    VencAllocateBufferParam bufferParam;

    memset(&baseConfig, 0 ,sizeof(VencBaseConfig));
    memset(&bufferParam, 0 ,sizeof(VencAllocateBufferParam));

    baseConfig.memops = MemAdapterGetOpsS();
    if (baseConfig.memops == NULL)
    {
        LOGE("Sunxi Cedar initalization fail: MemAdapterGetOpsS failed\n");
        return -EINVAL;
    }

    CdcMemOpen(baseConfig.memops);
    baseConfig.nInputWidth= width;
    baseConfig.nInputHeight = height;
    baseConfig.nStride = width;
    baseConfig.nDstWidth = width;
    baseConfig.nDstHeight = height;

    // Our input format is YUV422 multi plane (YM16) from sunxi-csi
    baseConfig.eInputFormat = VENC_PIXEL_YUV422SP;

    bufferParam.nSizeY = baseConfig.nInputWidth*baseConfig.nInputHeight;
    bufferParam.nSizeC = baseConfig.nInputWidth*baseConfig.nInputHeight;
    bufferParam.nBufferNum = 3;

    g_max_nSizeY = bufferParam.nSizeY;
    g_max_nSizeC = bufferParam.nSizeC;

    // We want to encode to H264
    g_pVideoEnc = VideoEncCreate(VENC_CODEC_H264);
    if (g_pVideoEnc == NULL)
    {
        CdcMemClose(baseConfig.memops);
        LOGE("Sunxi Cedar initalization fail: VideoEncCreate failed\n");
        return -EINVAL;
    }

    cedar_h264mode_init(g_pVideoEnc, width, height, fps);
    
    if(VideoEncInit(g_pVideoEnc, &baseConfig) != 0)
    {
        LOGE("Sunxi Cedar initalization fail: VideoEncInit failed\n");
        CdcMemClose(baseConfig.memops);
        VideoEncDestroy(g_pVideoEnc);
        return -EINVAL;
    }

    if(AllocInputBuffer(g_pVideoEnc, &bufferParam) != 0)
    {
        LOGE("Sunxi Cedar initalization fail: AllocInputBuffer failed\n");
        CdcMemClose(baseConfig.memops);
        VideoEncDestroy(g_pVideoEnc);
        return -EINVAL;
    }

    g_videoEncMemOps = baseConfig.memops;
    g_pts = 0;
    g_fps = fps;

    LOGD("Sunxi Cedar initalization success\n");
    return 0;
}

void cedar_hardware_deinit(void)
{
    LOGD("Deinitializing Sunxi Cedar hardware encoder...\n");

    if (g_pVideoEnc != NULL)
    {
        ReleaseAllocInputBuffer(g_pVideoEnc);
        VideoEncDestroy(g_pVideoEnc);
        releaseMb(&g_h264_func);
        g_pVideoEnc = NULL;
    }

    if (g_videoEncMemOps != NULL)
    {
        CdcMemClose(g_videoEncMemOps);
        g_videoEncMemOps = NULL;
    }

    LOGD("Sunxi Cedar deinitialization success\n");
}

int cedar_encode_get_sps_pps_buffer(char **out_buf, unsigned int *out_buf_len)
{
    VencHeaderData sps_pps_data;

    VideoEncGetParameter(g_pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
    *out_buf_len = sps_pps_data.nLength;
    *out_buf = (char *)malloc(*out_buf_len);
    if(*out_buf == NULL)
    {
        LOGE("Sunxi Cedar Get SPS Data failed: malloc failed\n");
        return -EINVAL;
    }
    memcpy(*out_buf, sps_pps_data.pBuffer, *out_buf_len);
    return 0;
}

int cedar_get_one_alloc_input_buffer(VencInputBuffer *inputBuffer)
{
    if(GetOneAllocInputBuffer(g_pVideoEnc, inputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: GetOneAllocInputBuffer failed\n");
        return -EINVAL;
    }

    return 0;
}

int cedar_release_one_alloc_input_buffer(VencInputBuffer *inputBuffer)
{
    if(ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: ReturnOneAllocInputBuffer failed\n");
        return -EINVAL;
    }

    return 0;
}

int cedar_encode_one_frame_yuv422sp(
    VencInputBuffer *inputBuffer,
    char **out_buf,
    unsigned int *out_buf_len)
{
    VencOutputBuffer outputBuffer;

    inputBuffer->bEnableCorp = 0;
    inputBuffer->sCropInfo.nLeft =  240;
    inputBuffer->sCropInfo.nTop  =  240;
    inputBuffer->sCropInfo.nWidth  =  240;
    inputBuffer->sCropInfo.nHeight =  240;

    if(FlushCacheAllocInputBuffer(g_pVideoEnc, inputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: FlushCacheAllocInputBuffer failed\n");
        ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer);
        return -EINVAL;
    }

    g_pts += 1*1000/g_fps;
    inputBuffer->nPts = g_pts;
    if(AddOneInputBuffer(g_pVideoEnc, inputBuffer))
    {
        LOGE("Sunxi Cedar encoding failed: AddOneInputBuffer failed\n");
        ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer);
        return -EINVAL;
    }

    int ret;
    if((ret = VideoEncodeOneFrame(g_pVideoEnc)) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: VideoEncodeOneFrame failed: %d\n", ret);
        ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer);
        return -EINVAL;
    }

    if(AlreadyUsedInputBuffer(g_pVideoEnc, inputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: AlreadyUsedInputBuffer failed\n");
        ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer);
        return -EINVAL;
    }

    if(ReturnOneAllocInputBuffer(g_pVideoEnc, inputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: ReturnOneAllocInputBuffer failed\n");
        return -EINVAL;
    }

    if(GetOneBitstreamFrame(g_pVideoEnc, &outputBuffer))
    {
        LOGE("Sunxi Cedar encoding failed: GetOneBitstreamFrame failed\n");
        return -EINVAL;
    }

    *out_buf_len = outputBuffer.nSize0;
    if(outputBuffer.nSize1)
        *out_buf_len += outputBuffer.nSize1;
    
    *out_buf = (char *)malloc(*out_buf_len);
    if(*out_buf == NULL)
    {
        LOGE("Sunxi Cedar encoding failed: malloc failed\n");
        FreeOneBitStreamFrame(g_pVideoEnc, &outputBuffer);
        return -EINVAL;
    }

    memcpy(*out_buf, outputBuffer.pData0, outputBuffer.nSize0);
    if(outputBuffer.nSize1)
        memcpy(*out_buf + outputBuffer.nSize0, outputBuffer.pData1, outputBuffer.nSize1);
    
    if(FreeOneBitStreamFrame(g_pVideoEnc, &outputBuffer) != 0)
    {
        LOGE("Sunxi Cedar encoding failed: FreeOneBitStreamFrame failed\n");
        free(*out_buf);
        return -EINVAL;
    }

    return 0;
}

int cedar_get_dmabuf_fd(char *buffer_virtaddr)
{
    return g_videoEncMemOps->get_dma_buf_fd(buffer_virtaddr);
}
