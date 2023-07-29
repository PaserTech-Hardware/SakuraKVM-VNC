#ifndef __SAKURAKVMD_CEDAR_H264_H__
#define __SAKURAKVMD_CEDAR_H264_H__

#include <vencoder.h>
#include <memoryAdapter.h>

#define ROI_NUM 4
#define ALIGN_XXB(y, x) (((x) + ((y)-1)) & ~((y)-1))

typedef struct {
    VencHeaderData          sps_pps_data;
    VencH264Param           h264Param;
    VencMBModeCtrl          h264MBMode;
    VencMBInfo              MBInfo;
    VencH264FixQP           fixQP;
    VencSuperFrameConfig    sSuperFrameCfg;
    VencH264SVCSkip         SVCSkip; // set SVC and skip_frame
    VencH264AspectRatio     sAspectRatio;
    VencH264VideoSignal     sVideoSignal;
    VencCyclicIntraRefresh  sIntraRefresh;
    VencROIConfig           sRoiConfig[ROI_NUM];
    VeProcSet               sVeProcInfo;
    VencOverlayInfoS        sOverlayInfo;
    VencSmartFun            sH264Smart;
}h264_func_t;

int initH264Func(h264_func_t *h264_func, unsigned int width, unsigned int height, unsigned int fps);
void cedar_h264mode_init(VideoEncoder *pVideoEnc, unsigned int width, unsigned int height, unsigned int fps);
int cedar_hardware_init(unsigned int width, unsigned int height, unsigned int fps);
void cedar_hardware_deinit(void);
int cedar_encode_get_sps_pps_buffer(char **out_buf, unsigned int *out_buf_len);
int cedar_get_one_alloc_input_buffer(VencInputBuffer *inputBuffer);
int cedar_release_one_alloc_input_buffer(VencInputBuffer *inputBuffer);
int cedar_encode_one_frame_yuv422sp(
    VencInputBuffer *inputBuffer,
    char **out_buf,
    unsigned int *out_buf_len);
int cedar_get_dmabuf_fd(char *buffer_virtaddr);

#endif
