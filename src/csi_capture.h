#ifndef __SAKURAKVMD_CSI_CAPTURE_H__
#define __SAKURAKVMD_CSI_CAPTURE_H__

int csi_capture_init(unsigned int width, unsigned int height);
int csi_get_num_req(void);
int csi_get_num_plane(void);
int csi_capture_queuebuf(int **buffer);
int csi_capture_queuebuf_again(int *buffer);
int csi_capture_start(void);
int csi_capture_frame_yuv422sp(int *buf_id);
void csi_capture_deinit(void);

#endif
