#include <rfb/rfb.h>
#include "logging.h"
#include "vncserver.h"

int vncserver_init(int argc, char *argv[])
{
    LOGI("Starting VNC server...");

    rfbLog = logging_vnc_redirect_info;
    rfbErr = logging_vnc_redirect_error;

    LOGD("Redirected libvncserver log functions");

    rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, 1920, 1080, 8, 3, 4);
    screen->frameBuffer = (char*)malloc(1920 * 1080 * 4);
    rfbInitServer(screen);
    rfbRunEventLoop(screen, -1, FALSE);

    return 0;
}