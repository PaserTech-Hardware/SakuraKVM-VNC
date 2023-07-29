#include <pthread.h>
#include "vncserver_types.h"
#include "list.h"
#include "video_buf_manager.h"

int sakuravnc_clientdata_create(sakuravnc_clientdata *clientdata)
{
    int ret;
    ret = list_create(&clientdata->keysdown_list);
    if(ret)
        return ret;
    
    clientdata->client_key_frame_buf = NULL;
    clientdata->client_p_frame_list_pos = NULL;

    return 0;
}

int sakuravnc_clientdata_destroy(sakuravnc_clientdata *clientdata)
{
    int ret;
    ret = list_destroy(clientdata->keysdown_list);
    if(ret)
        return ret;

    video_buf_dereference_key_frame_if_not_current(clientdata->client_key_frame_buf);

    return 0;
}
