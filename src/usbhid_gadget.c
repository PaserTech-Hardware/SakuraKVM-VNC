#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include "logging.h"
#include "list.h"
#include "usbhid_scancodes.h"
#include "usbhid_gadget.h"
#include "vncserver_types.h"

/* @brief HID modifier bits mapped to shift and control key codes */
static const uint8_t g_const_shiftCtrlMap[NUM_MODIFIER_BITS] = {
    0x02, // left shift
    0x20, // right shift
    0x01, // left control
    0x10  // right control
};
/* @brief HID modifier bits mapped to meta and alt key codes */
static const uint8_t g_const_metaAltMap[NUM_MODIFIER_BITS] = {
    0x08, // left meta
    0x80, // right meta
    0x04, // left alt
    0x40  // right alt
};

int g_keyboard_gadget_fd = -1;
int g_mouse_gadget_fd = -1;

int usbhid_gadget_keyboard_init(void)
{
    LOGI("Trying to open USB HID Gadget Keyboard...");
    g_keyboard_gadget_fd = open("/dev/hidg0", O_RDWR | O_NONBLOCK);
    if (g_keyboard_gadget_fd < 0)
    {
        LOGE("Failed to open USB HID Gadget Keyboard");
        return -1;
    }
    LOGI("USB HID Gadget Keyboard opened");
    return 0;
}

void usbhid_gadget_keyboard_close(void)
{
    if (g_keyboard_gadget_fd >= 0)
    {
        LOGI("Closing USB HID Gadget Keyboard...");
        close(g_keyboard_gadget_fd);
        g_keyboard_gadget_fd = -1;
        LOGI("USB HID Gadget Keyboard closed");
    }
}

int usbhid_gadget_mouse_init(void)
{
    LOGI("Trying to open USB HID Gadget Mouse...");
    g_mouse_gadget_fd = open("/dev/hidg1", O_RDWR | O_NONBLOCK);
    if (g_mouse_gadget_fd < 0)
    {
        LOGE("Failed to open USB HID Gadget Mouse");
        return -1;
    }
    LOGI("USB HID Gadget Mouse opened");
    return 0;
}

void usbhid_gadget_mouse_close(void)
{
    if (g_mouse_gadget_fd >= 0)
    {
        LOGI("Closing USB HID Gadget Mouse...");
        close(g_mouse_gadget_fd);
        g_mouse_gadget_fd = -1;
        LOGI("USB HID Gadget Mouse closed");
    }
}

int usbhid_gadget_write_keyboard(const char *report)
{
    unsigned int retry_count = HID_REPORT_RETRY_MAX;
    ssize_t ret;

    while(retry_count > 0)
    {
        ret = write(g_keyboard_gadget_fd, report, KEY_REPORT_LENGTH);
        if (ret == KEY_REPORT_LENGTH)
        {
            LOGV("USB HID Gadget Keyboard write success");
            return 0;
        }

        if(errno == EAGAIN || errno == ESHUTDOWN)
        {
            retry_count--;
            LOGV("USB HID Gadget Keyboard write needs retry, left retry count: %d", retry_count);
            continue;
        }

        // other errors, just break and no need for retry
        break;
    }
    
    LOGE("USB HID Gadget Keyboard write failed, errno: %d, %s", errno, strerror(errno));
    return -EIO;
}

int usbhid_gadget_write_mouse(const char *report)
{
    unsigned int retry_count = HID_REPORT_RETRY_MAX;
    ssize_t ret;

    while(retry_count > 0)
    {
        ret = write(g_mouse_gadget_fd, report, PTR_REPORT_LENGTH);
        if (ret == PTR_REPORT_LENGTH)
        {
            LOGV("USB HID Gadget Mouse write success");
            return 0;
        }

        if(errno == EAGAIN || errno == ESHUTDOWN)
        {
            retry_count--;
            LOGV("USB HID Gadget Mouse write needs retry, left retry count: %d", retry_count);
            continue;
        }

        // other errors, just break and no need for retry
        break;
    }
    
    LOGE("USB HID Gadget Mouse write failed, errno: %d, %s", errno, strerror(errno));
    return -EIO;
}

unsigned char rfb_key_to_gadget_modifier(rfbKeySym key)
{
    unsigned char mod = 0;

    if (key >= XK_Shift_L && key <= XK_Control_R)
    {
        mod = g_const_shiftCtrlMap[key - XK_Shift_L];
    }
    else if (key >= XK_Meta_L && key <= XK_Alt_R)
    {
        mod = g_const_metaAltMap[key - XK_Meta_L];
    }

    return mod;
}

unsigned char rfb_key_to_gadget_scancode(rfbKeySym key)
{
    unsigned char scancode = 0;

    if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'))
    {
        scancode = USBHID_KEY_A + ((key & 0x5F) - 'A');
    }
    else if (key >= '1' && key <= '9')
    {
        scancode = USBHID_KEY_1 + (key - '1');
    }
    else if (key >= XK_F1 && key <= XK_F12)
    {
        scancode = USBHID_KEY_F1 + (key - XK_F1);
    }
    else if (key >= XK_KP_F1 && key <= XK_KP_F4)
    {
        scancode = USBHID_KEY_F1 + (key - XK_KP_F1);
    }
    else if (key >= XK_KP_1 && key <= XK_KP_9)
    {
        scancode = USBHID_KEY_KP_1 + (key - XK_KP_1);
    }
    else
    {
        switch (key)
        {
            case XK_exclam:
                scancode = USBHID_KEY_1;
                break;
            case XK_at:
                scancode = USBHID_KEY_2;
                break;
            case XK_numbersign:
                scancode = USBHID_KEY_3;
                break;
            case XK_dollar:
                scancode = USBHID_KEY_4;
                break;
            case XK_percent:
                scancode = USBHID_KEY_5;
                break;
            case XK_asciicircum:
                scancode = USBHID_KEY_6;
                break;
            case XK_ampersand:
                scancode = USBHID_KEY_7;
                break;
            case XK_asterisk:
                scancode = USBHID_KEY_8;
                break;
            case XK_parenleft:
                scancode = USBHID_KEY_9;
                break;
            case XK_0:
            case XK_parenright:
                scancode = USBHID_KEY_0;
                break;
            case XK_Return:
                scancode = USBHID_KEY_RETURN;
                break;
            case XK_Escape:
                scancode = USBHID_KEY_ESC;
                break;
            case XK_BackSpace:
                scancode = USBHID_KEY_BACKSPACE;
                break;
            case XK_Tab:
            case XK_KP_Tab:
                scancode = USBHID_KEY_TAB;
                break;
            case XK_space:
            case XK_KP_Space:
                scancode = USBHID_KEY_SPACE;
                break;
            case XK_minus:
            case XK_underscore:
                scancode = USBHID_KEY_MINUS;
                break;
            case XK_plus:
            case XK_equal:
                scancode = USBHID_KEY_EQUAL;
                break;
            case XK_bracketleft:
            case XK_braceleft:
                scancode = USBHID_KEY_LEFTBRACE;
                break;
            case XK_bracketright:
            case XK_braceright:
                scancode = USBHID_KEY_RIGHTBRACE;
                break;
            case XK_backslash:
            case XK_bar:
                scancode = USBHID_KEY_BACKSLASH;
                break;
            case XK_colon:
            case XK_semicolon:
                scancode = USBHID_KEY_SEMICOLON;
                break;
            case XK_quotedbl:
            case XK_apostrophe:
                scancode = USBHID_KEY_APOSTROPHE;
                break;
            case XK_grave:
            case XK_asciitilde:
                scancode = USBHID_KEY_GRAVE;
                break;
            case XK_comma:
            case XK_less:
                scancode = USBHID_KEY_COMMA;
                break;
            case XK_period:
            case XK_greater:
                scancode = USBHID_KEY_DOT;
                break;
            case XK_slash:
            case XK_question:
                scancode = USBHID_KEY_SLASH;
                break;
            case XK_Caps_Lock:
                scancode = USBHID_KEY_CAPSLOCK;
                break;
            case XK_Print:
                scancode = USBHID_KEY_PRINT;
                break;
            case XK_Scroll_Lock:
                scancode = USBHID_KEY_SCROLLLOCK;
                break;
            case XK_Pause:
                scancode = USBHID_KEY_PAUSE;
                break;
            case XK_Insert:
            case XK_KP_Insert:
                scancode = USBHID_KEY_INSERT;
                break;
            case XK_Home:
            case XK_KP_Home:
                scancode = USBHID_KEY_HOME;
                break;
            case XK_Page_Up:
            case XK_KP_Page_Up:
                scancode = USBHID_KEY_PAGEUP;
                break;
            case XK_Delete:
            case XK_KP_Delete:
                scancode = USBHID_KEY_DELETE;
                break;
            case XK_End:
            case XK_KP_End:
                scancode = USBHID_KEY_END;
                break;
            case XK_Page_Down:
            case XK_KP_Page_Down:
                scancode = USBHID_KEY_PAGEDOWN;
                break;
            case XK_Right:
            case XK_KP_Right:
                scancode = USBHID_KEY_RIGHT;
                break;
            case XK_Left:
            case XK_KP_Left:
                scancode = USBHID_KEY_LEFT;
                break;
            case XK_Down:
            case XK_KP_Down:
                scancode = USBHID_KEY_DOWN;
                break;
            case XK_Up:
            case XK_KP_Up:
                scancode = USBHID_KEY_UP;
                break;
            case XK_Num_Lock:
                scancode = USBHID_KEY_NUMLOCK;
                break;
            case XK_KP_Enter:
                scancode = USBHID_KEY_KP_ENTER;
                break;
            case XK_KP_Equal:
                scancode = USBHID_KEY_KP_EQUAL;
                break;
            case XK_KP_Multiply:
                scancode = USBHID_KEY_KP_MULTIPLY;
                break;
            case XK_KP_Add:
                scancode = USBHID_KEY_KP_ADD;
                break;
            case XK_KP_Subtract:
                scancode = USBHID_KEY_KP_SUBTRACT;
                break;
            case XK_KP_Decimal:
                scancode = USBHID_KEY_KP_DECIMAL;
                break;
            case XK_KP_Divide:
                scancode = USBHID_KEY_KP_DIVIDE;
                break;
            case XK_KP_0:
                scancode = USBHID_KEY_KP_0;
                break;
        }
    }

    return scancode;
}

void rfb_key_event_handler(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    sakuravnc_clientdata *clientdata = (sakuravnc_clientdata *)cl->clientData;
    list_node_t *keysdown_list = clientdata->keysdown_list->head;
    list_node_t *keysdown_list_find;
    gadget_keyscan_item keyscan_item;
    int sendKeyboard = 0;
    unsigned char scancode;
    unsigned char modifier;
    unsigned int i;

    if(g_keyboard_gadget_fd < 0)
    {
        LOGV("USB HID Gadget Keyboard not opened");
        return;
    }

    if(down)
    {
        scancode = rfb_key_to_gadget_scancode(key);

        if(scancode)
        {
            for(keysdown_list_find = keysdown_list; keysdown_list_find != NULL; keysdown_list_find = keysdown_list_find->next)
            {
                if(((gadget_keyscan_item *)keysdown_list_find->data)->rfbkey == key)
                {
                    LOGV("Key %d already down", scancode);
                    break;
                }
            }

            if(keysdown_list_find == NULL)
            {
                LOGV("Key %d down", scancode);
                for(i = 2; i < KEY_REPORT_LENGTH; i ++)
                {
                    if(!clientdata->keyboardReport[i])
                    {
                        keyscan_item.rfbkey = key;
                        keyscan_item.scancode = scancode;
                        keyscan_item.reportpos = i;
                        list_put(clientdata->keysdown_list, &keyscan_item, sizeof(gadget_keyscan_item), NULL);
                        clientdata->keyboardReport[i] = scancode;
                        sendKeyboard = 1;
                        break;
                    }
                }
            }
        }
        else
        {
            modifier = rfb_key_to_gadget_modifier(key);

            if(modifier)
            {
                clientdata->keyboardReport[0] |= modifier;
                sendKeyboard = 1;
            }
        }
    }
    else
    {
        for(keysdown_list_find = keysdown_list; keysdown_list_find != NULL; keysdown_list_find = keysdown_list_find->next)
        {
            if(((gadget_keyscan_item *)keysdown_list_find->data)->rfbkey == key)
            {
                break;
            }
        }

        if(keysdown_list_find != NULL)
        {
            LOGV("Key %d up", ((gadget_keyscan_item *)keysdown_list_find->data)->scancode);
            clientdata->keyboardReport[((gadget_keyscan_item *)keysdown_list_find->data)->reportpos] = 0;
            list_delete(clientdata->keysdown_list, keysdown_list_find);
            sendKeyboard = 1;
        }
        else
        {
            modifier = rfb_key_to_gadget_modifier(key);

            if(modifier)
            {
                clientdata->keyboardReport[0] &= ~modifier;
                sendKeyboard = 1;
            }
        }
    }

    if(sendKeyboard)
    {
        usbhid_gadget_write_keyboard((const char *)clientdata->keyboardReport);
    }
}

void rfb_mouse_event_handler(int buttonMask, int x, int y, rfbClientPtr cl)
{
    sakuravnc_clientdata *clientdata = (sakuravnc_clientdata *)cl->clientData;

    if(g_mouse_gadget_fd < 0)
    {
        LOGV("USB HID Gadget Mouse not opened");
        return;
    }

    if (buttonMask > 4)
    {
        clientdata->pointerReport[0] = 0;
        if (buttonMask == 8)
        {
            clientdata->pointerReport[5] = 1;
        }
        else if (buttonMask == 16)
        {
            clientdata->pointerReport[5] = 0xff;
        }
    }
    else
    {
        clientdata->pointerReport[0] = ((buttonMask & 0x4) >> 1) |
                                  ((buttonMask & 0x2) << 1) |
                                  (buttonMask & 0x1);
        clientdata->pointerReport[5] = 0;
    }

    if (x >= 0 && x < cl->screen->width)
    {
        uint16_t xx = (uint16_t)(x * (SHRT_MAX + 1) / cl->screen->width);

        memcpy(&clientdata->pointerReport[1], &xx, 2);
    }

    if (y >= 0 && y < cl->screen->height)
    {
        uint16_t yy = (uint16_t)(y * (SHRT_MAX + 1) / cl->screen->height);

        memcpy(&clientdata->pointerReport[3], &yy, 2);
    }

    rfbDefaultPtrAddEvent(buttonMask, x, y, cl);
    usbhid_gadget_write_mouse((const char *)clientdata->pointerReport);
}
