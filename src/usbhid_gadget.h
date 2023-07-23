#ifndef __SAKURAKVMD_USBHID_GADGET_H__
#define __SAKURAKVMD_USBHID_GADGET_H__

#include <rfb/rfb.h>
#include <rfb/keysym.h>

#define NUM_MODIFIER_BITS 4
#define KEY_REPORT_LENGTH 8
#define PTR_REPORT_LENGTH 6
#define HID_REPORT_RETRY_MAX 5

typedef struct _gadget_keyscan_item {
    rfbKeySym rfbkey;
    unsigned char scancode;
    unsigned char reportpos;
} gadget_keyscan_item, *p_gadget_keyscan_item;

int usbhid_gadget_keyboard_init(void);
void usbhid_gadget_keyboard_close(void);
void rfb_key_event_handler(rfbBool down, rfbKeySym key, rfbClientPtr cl);

int usbhid_gadget_mouse_init(void);
void usbhid_gadget_mouse_close(void);
void rfb_mouse_event_handler(int buttonMask, int x, int y, rfbClientPtr cl);

#endif
