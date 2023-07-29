#ifndef __SAKURAKVMD_VIDEO_BUF_MANAGER__
#define __SAKURAKVMD_VIDEO_BUF_MANAGER__

#include "list.h"
#include "vncserver_types.h"

extern pthread_mutex_t g_current_key_frame_buf_mutex;

int video_buf_manager_init(void);
void video_buf_manager_destory(void);

void video_buf_reference_key_frame(video_key_frame_buf_t *client_key_frame_buf);
void video_buf_dereference_key_frame(video_key_frame_buf_t *client_key_frame_buf);

/**
 * @brief This is the main function to pack frame data for every client.
 * 
 * @param clientdata The client data structure.
 * @param buffer The buffer to store the packed frame data.
 * @param size The size of the packed frame data.
 *
 * @return errorcode, 0 for success, others for error.
 *
 * @note 
 * We do have the multi-client support, but there is only one H.264 hardware encoder.
 * This means that we should build a data structure to store the encoded result, and broadcast it to every client.
 * But if we store the encoded result for every frame, it will cost too much memory. That's not a good idea.
 *
 * We did some research on out project, and found that fact:
 * There is no need to store the all encoded frame data, while the client want's the newest frame data,
 * we can just store the newest key frame, and the newest P frame list after this key frame. 
 * (Every P frame needs the previous frame to decode, so here we need a list to store them)
 * 
 * So the problem become, we just need only one key frame buffer, and one P frame list buffer. WOW!
 *
 * But there is still a problem, the client may still using the old key frame buffer and P frame list buffer,
 * so we can't free them immediately. We should wait until the client stop using them.
 * To solve this problem, we can implement a reference count for the frame buffer structure.
 * Once the last client stop using them, we can free them.
 *
 * Wow, just say wow.
 *
 * This function will allocate memory for buffer, you should free it after use.
 */
int video_buf_pack_frame_data(sakuravnc_clientdata *clientdata, char **buffer, unsigned int *size);

#endif
