#include <rfb/rfb.h>
#include <vencoder.h>
#include <memoryAdapter.h>
#include <errno.h>
#include "logging.h"
#include "vncserver.h"
#include "cedar_h264.h"

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

char g_y_buf_test[1920 * 1080] = {0};
char g_c_buf_test[1920 * 1080 / 2] = {0};
FILE *file_debug = NULL;


rfbBool _rfbH264EncoderCallback(rfbClientPtr cl, char *buffer, size_t size)
{
    // todo: multi client with same framebuffer, dont encode twice
    if(buffer != NULL)
    {
        free(buffer);
    }

    size = size; // dummy for compiler warning

    if(cedar_encode_one_frame_yuv422sp(
        g_y_buf_test, 
        1920 * 1080, 
        g_c_buf_test,
        1920 * 1080 / 2,
        &cl->screen->h264Buffer,
        &cl->screen->h264BufferSize
        ) != 0)
    {
        LOGE("cedar_encode_one_frame_yuv422sp failed");
        return FALSE;
    }

	fwrite(cl->screen->h264Buffer, 1, cl->screen->h264BufferSize, file_debug);
	fflush(file_debug);

    return TRUE;
}

int vncserver_init(int argc, char *argv[])
{
    LOGI("Starting VNC server...");

    rfbLog = logging_vnc_redirect_info;
    rfbErr = logging_vnc_redirect_error;

    LOGD("Redirected libvncserver log functions");

    LOGD("Initalizing Sunxi Cedar hardware encoder...");
    cedar_hardware_init(1920, 1080, 30);

    rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, 1920, 1080, 8, 3, 4);
    screen->frameBuffer = (char*)malloc(1920 * 1080 * 4);

    SetXCursor2(screen);
    SetAlphaCursor(screen, 0);

    rfbInitServer(screen);
    screen->h264EncoderCallback = _rfbH264EncoderCallback;
    screen->h264Buffer = NULL;
    screen->h264BufferSize = 0;

	memset(g_y_buf_test, 0, sizeof(g_y_buf_test));
	memset(g_c_buf_test, 0, sizeof(g_c_buf_test));

	file_debug = fopen("cedar.h264", "wb");
	if(file_debug == NULL)
	{
		LOGE("fopen cedar.h264 failed");
		return -1;
	}

    rfbRunEventLoop(screen, -1, FALSE);

    return 0;
}