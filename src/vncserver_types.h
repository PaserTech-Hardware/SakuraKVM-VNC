#ifndef __SAKURAKVMD_VNCSERVER_TYPES__
#define __SAKURAKVMD_VNCSERVER_TYPES__

#include "list.h"
#include "usbhid_gadget.h"

typedef struct _video_key_frame_buf_t {
    list_ctx_t *p_frame_list;
    unsigned int reference_count;
    char *key_frame_buf;
    unsigned int key_frame_buf_size;
} video_key_frame_buf_t;

typedef struct _video_p_frame_buf_t {
    char *p_frame_buf;
    unsigned int p_frame_buf_size;
} video_p_frame_buf_t;

typedef struct _sakuravnc_clientdata {
    list_ctx_t *keysdown_list;
    unsigned char keyboardReport[KEY_REPORT_LENGTH];
    unsigned char pointerReport[PTR_REPORT_LENGTH];

    video_key_frame_buf_t *client_key_frame_buf;
    list_node_t *client_p_frame_list_pos;
} sakuravnc_clientdata, *p_sakuravnc_clientdata;

int sakuravnc_clientdata_create(sakuravnc_clientdata *clientdata);
int sakuravnc_clientdata_destroy(sakuravnc_clientdata *clientdata);

#endif
