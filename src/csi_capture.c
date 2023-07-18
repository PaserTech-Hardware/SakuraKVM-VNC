#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <sys/mman.h>
#include <sys/select.h>

// linux/videodev2.h has a bug that causes compile fail
// following two lines is to slove compile problem - error: field 'timestamp' has incomplete type
#include <bits/types/struct_timespec.h>
#include <bits/types/struct_timeval.h>

#include <linux/videodev2.h>
#include <errno.h>
#include "logging.h"

static int g_videodev_fd;

struct v4l2_buffer g_v4l2_buf[3] = {0};
struct v4l2_plane g_v4l2_planes_buffer[3][3];

struct v4l2_buffer g_buf;
struct v4l2_plane g_tmp_plane[3];

static int g_num_plane;
static int req_count;

static int xioctl(int fd, int request, void *arg)
{
    int r;
        do r = ioctl (fd, request, arg);
        while (-1 == r && EINTR == errno);
        return r;
}

static const char *buf_type_to_string(enum v4l2_buf_type type)
{
	switch (type) {
		case V4L2_BUF_TYPE_VIDEO_OUTPUT:
			return "OUTPUT";
		case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
			return "OUTPUT_MP";
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			return "CAPTURE";
		case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
			return "CAPTURE_MP";
		default:
			return "??";
	}
}

static void list_formats(int fd, enum v4l2_buf_type type)
{
	struct v4l2_fmtdesc fdesc;
	struct v4l2_frmsizeenum frmsize;

	LOGD("%s formats:", buf_type_to_string(type));

	memset(&fdesc, 0, sizeof(fdesc));
	fdesc.type = type;

	while (!ioctl(fd, VIDIOC_ENUM_FMT, &fdesc)) {
		LOGD("  %s", fdesc.description);

		memset(&frmsize, 0, sizeof(frmsize));
		frmsize.pixel_format = fdesc.pixelformat;

		while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {
			switch (frmsize.type) {
			case V4L2_FRMSIZE_TYPE_DISCRETE:
				LOGD("    %dx%d",
				    frmsize.discrete.width,
				    frmsize.discrete.height);
				break;
			case V4L2_FRMSIZE_TYPE_STEPWISE:
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
				LOGD("    %dx%d to %dx%d, step %+d%+d",
				    frmsize.stepwise.min_width,
				    frmsize.stepwise.min_height,
				    frmsize.stepwise.max_width,
				    frmsize.stepwise.max_height,
				    frmsize.stepwise.step_width,
				    frmsize.stepwise.step_height);
				break;
			}

			if (frmsize.type != V4L2_FRMSIZE_TYPE_DISCRETE)
				break;

			frmsize.index++;
		}

		fdesc.index++;
	}
}

int csi_get_num_req(void)
{
    return req_count;
}

int csi_get_num_plane(void)
{
    return g_num_plane;
}

int csi_capture_init(unsigned int width, unsigned int height)
{
    struct v4l2_capability caps = {0};
    struct v4l2_format fmt = {0};
    struct v4l2_requestbuffers req = {0};
    int i;

    // Open the csi capture device, our device is video0
    LOGI("Start Capture, Opening video device");
    g_videodev_fd = open("/dev/video0", O_RDWR);
    
    if (g_videodev_fd == -1)
    {
        // couldn't find capture device
        LOGE("Open video device failed");
        return -EINVAL;
    }

    if (0 != xioctl(g_videodev_fd, VIDIOC_QUERYCAP, &caps))
    {
        LOGE("Query video device capabilites failed");
        return -EINVAL;
    }

    LOGI("Capability information:");
	LOGI("cap.driver: \"%s\"", caps.driver);
	LOGI("cap.card: \"%s\"", caps.card);
	LOGI("cap.bus_info: \"%s\"", caps.bus_info);
	LOGI("cap.capabilities=0x%08X", caps.capabilities);
	if(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) LOGI("- VIDEO_CAPTURE");
	if(caps.capabilities & V4L2_CAP_VIDEO_OUTPUT)  LOGI("- VIDEO_OUTPUT");
	if(caps.capabilities & V4L2_CAP_VIDEO_OVERLAY) LOGI("- VIDEO_OVERLAY");
	if(caps.capabilities & V4L2_CAP_VBI_CAPTURE)   LOGI("- VBI_CAPTURE");
	if(caps.capabilities & V4L2_CAP_VBI_OUTPUT)    LOGI("- VBI_OUTPUT");
	if(caps.capabilities & V4L2_CAP_RDS_CAPTURE)   LOGI("- RDS_CAPTURE");
	if(caps.capabilities & V4L2_CAP_TUNER)         LOGI("- TUNER");
	if(caps.capabilities & V4L2_CAP_AUDIO)         LOGI("- AUDIO");
	if(caps.capabilities & V4L2_CAP_RADIO)         LOGI("- RADIO");
	if(caps.capabilities & V4L2_CAP_READWRITE)     LOGI("- READWRITE");
	if(caps.capabilities & V4L2_CAP_ASYNCIO)       LOGI("- ASYNCIO");
	if(caps.capabilities & V4L2_CAP_STREAMING)     LOGI("- STREAMING");
	if(caps.capabilities & V4L2_CAP_TIMEPERFRAME)  LOGI("- TIMEPERFRAME");

    list_formats(g_videodev_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

    // Sunxi CSI is a MPlane device
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    // We excepted YUV422 MPlane format, this is also named YUV422SP format
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422M;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (0 != xioctl(g_videodev_fd, VIDIOC_S_FMT, &fmt))
    {
        LOGE("Set video device pixel format failed");
        return -EINVAL;
    }

    // Now request the buffer
    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_DMABUF;

    if (0 != xioctl(g_videodev_fd, VIDIOC_REQBUFS, &req))
    {
        LOGE("Request buffer for the video device failed");
        return -EINVAL;
    }

    req_count = req.count;

    memset(g_v4l2_buf, 0, sizeof(struct v4l2_buffer));
    memset(g_v4l2_planes_buffer, 0, sizeof(g_v4l2_planes_buffer));
    for(i = 0; i < req_count; i++) {
        g_v4l2_buf[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        g_v4l2_buf[i].memory = V4L2_MEMORY_DMABUF;
        g_v4l2_buf[i].m.planes = g_v4l2_planes_buffer[i];
        g_v4l2_buf[i].length = fmt.fmt.pix_mp.num_planes;
        g_v4l2_buf[i].index = i;

        if (-1 == ioctl (g_videodev_fd, VIDIOC_QUERYBUF, &g_v4l2_buf[i])) {
            LOGE("Query video device buffer failed");
            return -EINVAL;
        }
    }

    g_num_plane = fmt.fmt.pix_mp.num_planes;
    
    return 0;
}

int csi_capture_queuebuf(int **buffer)
{
    int i, j;

    memset(&g_v4l2_buf, 0, sizeof(struct v4l2_buffer));
    for (i = 0; i < req_count; i++) {

        g_v4l2_buf[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        g_v4l2_buf[i].memory = V4L2_MEMORY_DMABUF;
        g_v4l2_buf[i].length = g_num_plane;
        g_v4l2_buf[i].index = i;
        g_v4l2_buf[i].m.planes = g_v4l2_planes_buffer[i];

        for (j = 0; j < g_num_plane; j++) {
            g_v4l2_buf[i].m.planes[j].m.fd = buffer[i][j];
            g_v4l2_buf[i].m.planes[j].length = g_v4l2_planes_buffer[i][j].length;
            g_v4l2_buf[i].m.planes[j].bytesused = 0;

            LOGD("csi: queue buf, plane[%d] fd = %d, length = %d", j, g_v4l2_buf[i].m.planes[j].m.fd, g_v4l2_planes_buffer[i][j].length);
        }

        LOGD("csi: queue buffer req %d", i);

        if (xioctl(g_videodev_fd, VIDIOC_QBUF, &g_v4l2_buf[i]) < 0)
        {
            LOGE("Queue video device buffer failed with errno %d: %s", errno, strerror(errno));
            return -EINVAL;
        }
    }

    return 0;
}

int csi_capture_start(void)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if(0 != xioctl(g_videodev_fd, VIDIOC_STREAMON, &type))
    {
        LOGE("Start video device stream failed");
        return -EINVAL;
    }
    
    return 0;
}

int csi_capture_frame_yuv422sp(int *buf_id)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(g_videodev_fd, &fds);
    struct timeval tv = {5, 0};

    LOGV("csi capture: waiting for frame...");
    int r = select(g_videodev_fd+1, &fds, NULL, NULL, &tv);
    if(r < 0)
    {
        LOGE("csi capture select error");
        return -EINVAL;
    }
	if(r == 0)
	{
		LOGE("csi capture select timeout\n");
		return -ETIMEDOUT;
	}

	LOGV("csi capture: retrieving frame...");
    memset(&g_buf, 0, sizeof(g_buf));
    g_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    g_buf.memory = V4L2_MEMORY_DMABUF;
    g_buf.m.planes = g_tmp_plane;
    g_buf.length = g_num_plane;
    if(0 != xioctl(g_videodev_fd, VIDIOC_DQBUF, &g_buf))
    {
        LOGE("csi capture: retrieve frame failed");
        return -EINVAL;
	}
    *buf_id = g_buf.index;
	LOGV("csi capture: retrieved frame");

    return 0;
}

int csi_capture_queuebuf_again(int *buffer)
{
    int i;

    for(i = 0; i < g_num_plane; i++)
    {
        g_buf.m.planes[i].m.fd = buffer[i];
        g_buf.m.planes[i].length = g_v4l2_planes_buffer[g_buf.index][i].length;
        g_buf.m.planes[i].bytesused = 0;
    }

    if (xioctl(g_videodev_fd, VIDIOC_QBUF, &g_buf) < 0)
    {
        LOGE("Queue video device buffer again failed");
        return -EINVAL;
    }

    return 0;
}

void csi_capture_deinit(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    // stop the stream
    if(0 != xioctl(g_videodev_fd, VIDIOC_STREAMOFF, &type))
    {
        LOGE("Stop video device stream failed");
        return;
    }

    // close the device
    close(g_videodev_fd);
}
