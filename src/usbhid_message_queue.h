#ifndef __SAKURAKVMD_USBHID_MESSAGE_QUEUE_H__
#define __SAKURAKVMD_USBHID_MESSAGE_QUEUE_H__

#include "usbhid_gadget_defs.h"

typedef struct _usbhid_message_struct {
    union {
        char keyboard_report_buf[KEY_REPORT_LENGTH];
        char mouse_report_buf[PTR_REPORT_LENGTH];
    } _dummy ;
} usbhid_message_struct;

typedef enum _usbhid_message_type {
    USBHID_MESSAGE_TYPE_KEYBOARD = 0,
    USBHID_MESSAGE_TYPE_MOUSE,
    USBHID_MESSAGE_TYPE_MAX
} usbhid_message_type;

void usbhid_message_consume_thread_init(void);
void usbhid_message_consume_thread_destory(void);
int usbhid_message_queue_push(usbhid_message_type type, usbhid_message_struct *message);

#endif
