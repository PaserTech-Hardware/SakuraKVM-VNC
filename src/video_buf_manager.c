#include <pthread.h>
#include "logging.h"
#include "list.h"
#include "video_buf_manager.h"
#include "vncserver_types.h"
#include "csi_capture.h"
#include "cedar_h264.h"

VencInputBuffer *g_requestbuf = NULL;
int **g_cedarVirtBufferFd = NULL;

video_key_frame_buf_t *g_current_key_frame_buf = NULL;
pthread_mutex_t g_current_key_frame_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

int g_video_buf_manager_still_run = 0;
pthread_t g_videobuf_render_thread;

int video_buf_capture_encode(char **encode_buf, unsigned int *encode_size)
{
    int buf_id, i;

    if(csi_capture_frame_yuv422sp(&buf_id) != 0)
	{
		LOGE("csi_capture_frame_yuv422sp failed");
		return FALSE;
	}

    if(cedar_encode_one_frame_yuv422sp(
		&g_requestbuf[buf_id],
        encode_buf,
        encode_size
        ) != 0)
    {
        LOGE("cedar_encode_one_frame_yuv422sp failed");
        return FALSE;
    }

	if(cedar_get_one_alloc_input_buffer(&g_requestbuf[buf_id]))
	{
		LOGE("cedar_get_one_alloc_input_buffer failed");
		return FALSE;
	}

	for(i = 0; i < csi_get_num_plane(); i++)
	{
		switch(i)
		{
			case 0:
				g_cedarVirtBufferFd[buf_id][i] = cedar_get_dmabuf_fd((char*)g_requestbuf[buf_id].pAddrVirY);
				break;
			case 1:
				g_cedarVirtBufferFd[buf_id][i] = cedar_get_dmabuf_fd((char*)g_requestbuf[buf_id].pAddrVirC);
				break;
			default:
				LOGE("unknown plane_num %d", i);
				cedar_release_one_alloc_input_buffer(&g_requestbuf[buf_id]);
				return FALSE;
		}
	}

	if(csi_capture_queuebuf_again(g_cedarVirtBufferFd[buf_id]))
	{
		LOGE("csi_capture_queuebuf_again failed");
		return FALSE;
	}

    return TRUE;
}

int video_buf_handle_key_frame(char *encode_buf, unsigned int encode_size)
{
    video_key_frame_buf_t *key_frame_buf = NULL;
    video_p_frame_buf_t *p_frame_buf = NULL;
    list_node_t *node = NULL;

    key_frame_buf = (video_key_frame_buf_t *)malloc(sizeof(video_key_frame_buf_t));
    if(key_frame_buf == NULL)
    {
        LOGW("Failed to allocate memory for key frame buffer");
        return FALSE;
    }

    if(list_create(&key_frame_buf->p_frame_list) != 0)
    {
        LOGW("Failed allocate create P frame list");
        free(key_frame_buf);
        return FALSE;
    }

    key_frame_buf->key_frame_buf = encode_buf;
    key_frame_buf->key_frame_buf_size = encode_size;
    key_frame_buf->reference_count = 0;

    pthread_mutex_lock(&g_current_key_frame_buf_mutex);
    if(g_current_key_frame_buf != NULL)
    {
        if(g_current_key_frame_buf->reference_count == 0)
        {
            LOGV("Current key frame buffer reference count is 0, free it and replace it directly");
            node = g_current_key_frame_buf->p_frame_list->head;
            while(node != NULL)
            {
                p_frame_buf = (video_p_frame_buf_t *)node->data;
                node = node->next;
                free(p_frame_buf->p_frame_buf);
            }
            if(list_destroy(g_current_key_frame_buf->p_frame_list) != 0)
            {
                LOGW("Failed to destroy P frame list, memory leak will cause!");
            }
            free(g_current_key_frame_buf->key_frame_buf);
            free(g_current_key_frame_buf);
        }
        else
        {
            LOGV("Current key frame buffer reference count is %d, cant free it now", g_current_key_frame_buf->reference_count);
        }
    }
    g_current_key_frame_buf = key_frame_buf;
    pthread_mutex_unlock(&g_current_key_frame_buf_mutex);

    return TRUE;
}

int video_buf_handle_p_frame(char *encode_buf, unsigned int encode_size)
{
    // for p frame, directly append it to current key frame buffer
    video_p_frame_buf_t p_frame_buf;

    pthread_mutex_lock(&g_current_key_frame_buf_mutex);
    if(g_current_key_frame_buf == NULL)
    {
        LOGV("No current key frame buffer, discard P frame");
        free(encode_buf);
        pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
        return FALSE;
    }

    p_frame_buf.p_frame_buf = encode_buf;
    p_frame_buf.p_frame_buf_size = encode_size;

    if(list_put(g_current_key_frame_buf->p_frame_list, &p_frame_buf, sizeof(video_p_frame_buf_t), NULL) != 0)
    {
        LOGW("Failed to append P frame to list");
        free(encode_buf);
        pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
        return FALSE;
    }
    pthread_mutex_unlock(&g_current_key_frame_buf_mutex);

    return TRUE;
}

int video_buf_render_next_frame(void)
{
    char *encode_buf = NULL;
    unsigned int encode_buf_size = 0;

    if(video_buf_capture_encode(&encode_buf, &encode_buf_size) == FALSE)
    {
        LOGW("Failed to capture and encode video frame");
        return FALSE;
    }

    if(encode_buf_size < 5)
    {
        LOGW("Invalid encoded video frame size %d", encode_buf_size);
        free(encode_buf);
        return FALSE;
    }
    
    // 0x00 0x00 0x00 0x01 0x?? (?? = 0x65 for IDR frame, 0x61 for P frame)
    if(memcmp(encode_buf, "\x00\x00\x00\x01", 4) != 0)
    {
        LOGW("Invalid encoded video frame header");
        free(encode_buf);
        return FALSE;
    }

    switch(encode_buf[4])
    {
        case 0x65:
            LOGV("Encoded IDR frame");
            if(video_buf_handle_key_frame(encode_buf, encode_buf_size) == FALSE)
            {
                LOGW("Failed to handle key frame");
                free(encode_buf);
                return FALSE;
            }
            break;
        case 0x41:
            LOGV("Encoded P frame");
            if(video_buf_handle_p_frame(encode_buf, encode_buf_size) == FALSE)
            {
                LOGW("Failed to handle P frame");
                free(encode_buf);
                return FALSE;
            }
            break;
        default:
            LOGW("Unknown encoded video frame type 0x%02x", encode_buf[4]);
            free(encode_buf);
            return FALSE;
    }

    return 0;
}

void *video_buf_render_thread(void *dummy)
{
    (void)dummy; // unused variable for compiler warning

    while(g_video_buf_manager_still_run)
    {
        if(video_buf_render_next_frame() != 0)
        {
            LOGD("Failed to render next video frame");
        }
    }

    return NULL;
}

void video_buf_reference_key_frame(video_key_frame_buf_t *client_key_frame_buf)
{
    if(client_key_frame_buf == NULL)
        return;

    client_key_frame_buf->reference_count++;
}

void video_buf_dereference_key_frame(video_key_frame_buf_t *client_key_frame_buf)
{
    if(client_key_frame_buf == NULL)
        return;

    client_key_frame_buf->reference_count--;
    if(client_key_frame_buf->reference_count == 0)
    {
        LOGV("Client key frame buffer reference count is 0, free it");
        list_node_t *node = client_key_frame_buf->p_frame_list->head;
        video_p_frame_buf_t *p_frame_buf = NULL;
        while(node != NULL)
        {
            p_frame_buf = (video_p_frame_buf_t *)node->data;
            node = node->next;
            free(p_frame_buf->p_frame_buf);
        }
        if(list_destroy(client_key_frame_buf->p_frame_list) != 0)
        {
            LOGW("Failed to destroy P frame list, memory leak will cause!");
        }
        free(client_key_frame_buf->key_frame_buf);
        free(client_key_frame_buf);
    }
}

void video_buf_dereference_key_frame_if_not_current(video_key_frame_buf_t *client_key_frame_buf)
{
    pthread_mutex_lock(&g_current_key_frame_buf_mutex);
    if(client_key_frame_buf != g_current_key_frame_buf)
    {
        video_buf_dereference_key_frame(client_key_frame_buf);
    }
    pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
}

int video_buf_pack_frame_data(sakuravnc_clientdata *clientdata, char **buffer, unsigned int *size)
{
    video_key_frame_buf_t *client_key_frame_buf = clientdata->client_key_frame_buf;
    list_node_t *client_p_frame_node = clientdata->client_p_frame_list_pos;
    list_node_t *tmp_p_frame_node, *_p_frame_node;

    unsigned int total_frame_length = 0;
    unsigned int frame_buf_pos = 0;
    int key_buf_changed = 0;
    video_key_frame_buf_t *dereference_key_frame_buf = NULL;
    video_key_frame_buf_t *reference_key_frame_buf = NULL;

    pthread_mutex_lock(&g_current_key_frame_buf_mutex);
    if(g_current_key_frame_buf == NULL)
    {
        LOGD("Global current key frame buffer is NULL, no frame data to pack");
        pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
        return FALSE;
    }

    if(client_key_frame_buf != g_current_key_frame_buf)
    {
        dereference_key_frame_buf = client_key_frame_buf;
        client_key_frame_buf = g_current_key_frame_buf;
        reference_key_frame_buf = g_current_key_frame_buf;
        key_buf_changed = 1;
        total_frame_length = client_key_frame_buf->key_frame_buf_size;
        tmp_p_frame_node = client_key_frame_buf->p_frame_list->head;
    }
    else
    {
        total_frame_length = 0;
        tmp_p_frame_node = (client_p_frame_node != NULL ? client_p_frame_node->next : client_key_frame_buf->p_frame_list->head);
    }

    // we need send all data in P frame list, so count the total frame length
    _p_frame_node = tmp_p_frame_node;
    while(_p_frame_node != NULL)
    {
        total_frame_length += ((video_p_frame_buf_t *)_p_frame_node->data)->p_frame_buf_size;
        _p_frame_node = _p_frame_node->next;
    }

    if(total_frame_length == 0)
    {
        LOGD("No frame data to pack");
        pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
        return FALSE;
    }

    *buffer = (char *)malloc(total_frame_length);
    if(*buffer == NULL)
    {
        LOGW("Failed to allocate memory for frame data");
        pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
        return FALSE;
    }

    // copy key frame data if changed
    if(key_buf_changed)
    {
        memcpy(*buffer, client_key_frame_buf->key_frame_buf, client_key_frame_buf->key_frame_buf_size);
        frame_buf_pos += client_key_frame_buf->key_frame_buf_size;
    }

    // copy all P frame data
    _p_frame_node = tmp_p_frame_node;
    while(_p_frame_node != NULL)
    {
        memcpy(*buffer + frame_buf_pos, ((video_p_frame_buf_t *)_p_frame_node->data)->p_frame_buf, ((video_p_frame_buf_t *)_p_frame_node->data)->p_frame_buf_size);
        frame_buf_pos += ((video_p_frame_buf_t *)_p_frame_node->data)->p_frame_buf_size;
        _p_frame_node = _p_frame_node->next;
    }

    // update client data
    clientdata->client_key_frame_buf = client_key_frame_buf;
    clientdata->client_p_frame_list_pos = client_key_frame_buf->p_frame_list->tail;

    // update reference count
    if(dereference_key_frame_buf != NULL)
    {
        video_buf_dereference_key_frame(dereference_key_frame_buf);
    }
    if(reference_key_frame_buf != NULL)
    {
        video_buf_reference_key_frame(reference_key_frame_buf);
    }

    pthread_mutex_unlock(&g_current_key_frame_buf_mutex);
    *size = total_frame_length;

    return TRUE;
}

int video_buf_manager_init(void)
{
    unsigned int req_cnt, plane_num, i, j;

    LOGD("Initalizing Sunxi Cedar hardware encoder...");
    cedar_hardware_init(1920, 1080, 30);

	LOGD("Initalizing Sunxi CSI capture...");
	csi_capture_init(1920, 1080);
	req_cnt = csi_get_num_req();
	plane_num = csi_get_num_plane();
	g_requestbuf = (VencInputBuffer*)malloc(sizeof(VencInputBuffer) * req_cnt);
	if(!g_requestbuf)
	{
		LOGE("malloc g_requestbuf failed");
		return -1;
	}
	g_cedarVirtBufferFd = (int**)malloc(sizeof(int*) * req_cnt);
	if(!g_cedarVirtBufferFd)
	{
		LOGE("malloc g_cedarVirtBufferFd failed");
		return -1;
	}
	for(i = 0; i < req_cnt; i++)
	{
		g_cedarVirtBufferFd[i] = (int*)malloc(sizeof(int) * plane_num);
		if(!g_cedarVirtBufferFd[i])
		{
			LOGE("malloc g_cedarVirtBufferFd[%d] failed", i);
			free(g_requestbuf);
			free(g_cedarVirtBufferFd);
			return -1;
		}
		cedar_get_one_alloc_input_buffer(&g_requestbuf[i]);
		for(j = 0; j < plane_num; j++)
		{
			switch(j)
			{
				case 0:
					g_cedarVirtBufferFd[i][j] = cedar_get_dmabuf_fd((char*)g_requestbuf[i].pAddrVirY);
					LOGD("cedar memory buf id %d plane %d pAddrVirY = %p, pAddrPhyY = %p, dmabuf_fd = %d", i, j, g_requestbuf[i].pAddrVirY, g_requestbuf[i].pAddrPhyY, g_cedarVirtBufferFd[i][j]);
					break;
				case 1:
					g_cedarVirtBufferFd[i][j] = cedar_get_dmabuf_fd((char*)g_requestbuf[i].pAddrVirC);
					LOGD("cedar memory buf id %d plane %d pAddrVirC = %p, pAddrPhyC = %p, dmabuf_fd = %d", i, j, g_requestbuf[i].pAddrVirC, g_requestbuf[i].pAddrPhyC, g_cedarVirtBufferFd[i][j]);
					break;
				default:
					LOGE("unknown plane_num %d", j);
					cedar_release_one_alloc_input_buffer(&g_requestbuf[i]);
					free(g_requestbuf);
					free(g_cedarVirtBufferFd);
					return -1;
			}
		}
	}
	csi_capture_queuebuf(g_cedarVirtBufferFd);
	csi_capture_start();

    g_video_buf_manager_still_run = 1;
    if(pthread_create(&g_videobuf_render_thread, NULL, video_buf_render_thread, NULL) != 0)
    {
        LOGE("Failed to create video buffer render thread");
        return -1;
    }

    return 0;
}

void video_buf_manager_destory(void)
{
    unsigned int i, req_cnt;
    g_video_buf_manager_still_run = 0;
    pthread_join(g_videobuf_render_thread, NULL);

    LOGD("Stopping Sunxi CSI capture...");
    csi_capture_deinit();

    LOGD("Stopping Sunxi Cedar hardware encoder...");
    cedar_hardware_deinit();

    req_cnt = csi_get_num_req();

    for(i = 0; i < req_cnt; i++)
    {
        free(g_cedarVirtBufferFd[i]);
    }
    free(g_cedarVirtBufferFd);
    free(g_requestbuf);

    LOGD("Video buffer manager destoryed");
}
