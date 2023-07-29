#include <rfb/rfb.h>
#include <errno.h>
#include "vncserver_types.h"
#include "logging.h"
#include "vncserver.h"
#include "cedar_h264.h"
#include "usbhid_gadget.h"
#include "usbhid_message_queue.h"
#include "video_buf_manager.h"

static void SetXCursor2(rfbScreenInfoPtr rfbScreen)
{
	int width=13,height=22;
	char cursor[]=
		" xx          "
		" x x         "
		" x  x        "
		" x   x       "
		" x    x      "
		" x     x     "
		" x      x    "
		" x       x   "
		" x     xx x  "
		" x x   x xxx "
		" x xx  x   x "
		" xx x   x    "
		" xx  x  x    "
		" x    x  x   "
		" x    x  x   "
		"       x  x  "
		"        x  x "
		"        x  x "
		"         xx  "
		"             "
		"             ",
	     mask[]=
		"xxx          "
		"xxxx         "
		"xxxxx        "
		"xxxxxx       "
		"xxxxxxx      "
		"xxxxxxxx     "
		"xxxxxxxxx    "
		"xxxxxxxxxx   "
		"xxxxxxxxxxx  "
		"xxxxxxxxxxxx "
		"xxxxxxxxxxxxx"
		"xxxxxxxxxxxxx"
		"xxxxxxxxxx  x"
		"xxxxxxxxxx   "
		"xxx  xxxxxx  "
		"xxx  xxxxxx  "
		"xx    xxxxxx "
		"       xxxxx "
		"       xxxxxx"
		"        xxxxx"
		"         xxx "
		"             ";
	rfbCursorPtr c;
	
	c=rfbMakeXCursor(width,height,cursor,mask);
	c->xhot=0;c->yhot=0;

	rfbSetCursor(rfbScreen, c);
}

static void SetAlphaCursor(rfbScreenInfoPtr screen,int mode)
{
	int i,j;
	rfbCursorPtr c = screen->cursor;
	int maskStride;

	if(!c)
		return;

	maskStride = (c->width+7)/8;

	if(c->alphaSource) {
		free(c->alphaSource);
		c->alphaSource=NULL;
	}

	if(mode==0)
		return;

	c->alphaSource = (unsigned char*)malloc(c->width*c->height);
	if (!c->alphaSource) return;

	for(j=0;j<c->height;j++)
		for(i=0;i<c->width;i++) {
			unsigned char value=0x100*i/c->width;
			rfbBool masked=(c->mask[(i/8)+maskStride*j]<<(i&7))&0x80;
			c->alphaSource[i+c->width*j]=(masked?(mode==1?value:0xff-value):0);
		}
	if(c->cleanupMask)
		free(c->mask);
	c->mask=(unsigned char*)rfbMakeMaskFromAlphaSource(c->width,c->height,c->alphaSource);
	c->cleanupMask=TRUE;
}

FILE *file_debug = NULL;

rfbBool _rfbH264EncoderCallback(rfbClientPtr cl, char *buffer, size_t size)
{
	char *sps_head_buffer;
	unsigned int sps_head_buffer_size;

    // todo: multi client with same framebuffer, dont encode twice
    if(buffer != NULL)
    {
        free(buffer);
		cl->screen->h264Buffer = NULL;
		cl->screen->h264BufferSize = 0;
    }

    size = size; // dummy for compiler warning

	if(video_buf_pack_frame_data(cl->clientData, &cl->screen->h264Buffer, &cl->screen->h264BufferSize) == FALSE)
	{
		return FALSE;
	}

	if(cl->isSPS_PPS_Sent == FALSE)
	{
		if(cedar_encode_get_sps_pps_buffer(&sps_head_buffer, &sps_head_buffer_size) != 0)
		{
			LOGE("cedar_encode_get_sps_pps_buffer failed");
			return FALSE;
		}
		sps_head_buffer = realloc(sps_head_buffer, sps_head_buffer_size + cl->screen->h264BufferSize);
		if(!sps_head_buffer)
		{
			LOGE("realloc sps_head_buffer failed");
			return FALSE;
		}
		memcpy(sps_head_buffer + sps_head_buffer_size, cl->screen->h264Buffer, cl->screen->h264BufferSize);
		free(cl->screen->h264Buffer);
		cl->screen->h264Buffer = sps_head_buffer;
		cl->screen->h264BufferSize += sps_head_buffer_size;
		cl->isSPS_PPS_Sent = TRUE;

		// fixme: WTF why does this makes decoder output immediately without 16 frames delay?
		// NAL Header: 00 00 00 01 -> 4 bytes
		// SPS Header: ?? ?? XX ?? -> offset 3 which used for "level"
		cl->screen->h264Buffer[7] = 30;

		return TRUE;
	}

    return TRUE;
}

void vncserver_client_gone(rfbClientPtr cl)
{
	LOGV("Client %s gone", cl->host);

	if(!cl->clientData)
		return;

    if(sakuravnc_clientdata_destroy(cl->clientData) != 0)
	{
		LOGW("Cannot destroy client data %d: %s, memory leak will cause!", errno, strerror(errno));
	}

	free(cl->clientData);
	cl->clientData = NULL;
}

enum rfbNewClientAction vncserver_new_client(rfbClientPtr cl)
{
	LOGV("New client connected from %s", cl->host);

	cl->clientGoneHook = vncserver_client_gone;
	cl->clientData = malloc(sizeof(sakuravnc_clientdata));
	if(!cl->clientData)
	{
		LOGE("Cannot allocate memory for client data");
		return RFB_CLIENT_REFUSE;
	}

	if(sakuravnc_clientdata_create(cl->clientData) != 0)
	{
		LOGE("Cannot initalize client data %d: %s", errno, strerror(errno));
		free(cl->clientData);
		cl->clientData = NULL;
		return RFB_CLIENT_REFUSE;
	}

	return RFB_CLIENT_ACCEPT;
}

int vncserver_init(int argc, char *argv[])
{	
	LOGI("Starting VNC server...");

    rfbLog = logging_vnc_redirect_info;
    rfbErr = logging_vnc_redirect_error;

    LOGD("Redirected libvncserver log functions");

	LOGD("Initalizing USB HID gadget message queue...");
	usbhid_message_consume_thread_init();

	LOGD("Initalizing USB HID keyboard gadget...");
	usbhid_gadget_keyboard_init();

	LOGD("Initalizing USB HID mouse gadget...");
	usbhid_gadget_mouse_init();

	LOGD("Initalizing Video Capture Manager...");
	video_buf_manager_init();

    rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, 1920, 1080, 8, 3, 4);
    screen->frameBuffer = (char*)malloc(1920 * 1080 * 4);
    screen->desktopName = "SakuraKVM-VNC";

    SetXCursor2(screen);
    SetAlphaCursor(screen, 0);

    rfbInitServer(screen);
    screen->h264EncoderCallback = _rfbH264EncoderCallback;
    screen->h264Buffer = NULL;
    screen->h264BufferSize = 0;

	screen->kbdAddEvent = rfb_key_event_handler;
	screen->ptrAddEvent = rfb_mouse_event_handler;
	screen->newClientHook = vncserver_new_client;

    rfbRunEventLoop(screen, -1, FALSE);

    return 0;
}