#include <errno.h>
#include <pthread.h>

#include "list.h"
#include "usbhid_message_queue.h"
#include "usbhid_gadget.h"
#include "logging.h"

list_ctx_t *g_usbhid_message_keyboard_list;
list_ctx_t *g_usbhid_message_mouse_list;

pthread_mutex_t g_usbhid_message_keyboard_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_usbhid_message_mouse_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t g_usbhid_message_keyboard_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t g_usbhid_message_mouse_cond = PTHREAD_COND_INITIALIZER;

pthread_t g_keyboard_thread, g_mouse_thread;

int still_run = 1;

int usbhid_message_queue_init(void)
{
    int ret;
    ret = list_create(&g_usbhid_message_keyboard_list);
    if(ret)
        return ret;
    ret = list_create(&g_usbhid_message_mouse_list);
    if(ret) {
        list_destroy(g_usbhid_message_keyboard_list);
        return ret;
    }
    return 0;
}

void usbhid_message_queue_destory(void)
{
    list_destroy(g_usbhid_message_keyboard_list);
    list_destroy(g_usbhid_message_mouse_list);
}

int usbhid_message_queue_push(usbhid_message_type type, usbhid_message_struct *message)
{
    int ret;
    switch(type) {
        case USBHID_MESSAGE_TYPE_KEYBOARD:
            pthread_mutex_lock(&g_usbhid_message_keyboard_mutex);
            ret = list_put(g_usbhid_message_keyboard_list, message, sizeof(usbhid_message_struct), NULL);
            pthread_cond_signal(&g_usbhid_message_keyboard_cond);
            pthread_mutex_unlock(&g_usbhid_message_keyboard_mutex);
            return ret;
        case USBHID_MESSAGE_TYPE_MOUSE:
            pthread_mutex_lock(&g_usbhid_message_mouse_mutex);
            ret = list_put(g_usbhid_message_mouse_list, message, sizeof(usbhid_message_struct), NULL);
            pthread_cond_signal(&g_usbhid_message_mouse_cond);
            pthread_mutex_unlock(&g_usbhid_message_mouse_mutex);
            return ret;
        default:
            break;
    }

    LOGW("Unknown USB HID gadget message type %d", type);
    return -EINVAL;
}

void *usbhid_message_consume_keyboard_thread(void *dummy)
{
    usbhid_message_struct *message;
    int ret;

    (void)dummy; // unused variable for compiler warning

    while(still_run) {
        pthread_mutex_lock(&g_usbhid_message_keyboard_mutex);
        while(g_usbhid_message_keyboard_list->head == NULL)
            pthread_cond_wait(&g_usbhid_message_keyboard_cond, &g_usbhid_message_keyboard_mutex);
        message = (usbhid_message_struct *) g_usbhid_message_keyboard_list->head->data;
        pthread_mutex_unlock(&g_usbhid_message_keyboard_mutex);

        ret = usbhid_gadget_write_keyboard(message->_dummy.keyboard_report_buf);

        pthread_mutex_lock(&g_usbhid_message_keyboard_mutex);
        if(ret == 0)
        {
            list_delete(g_usbhid_message_keyboard_list, g_usbhid_message_keyboard_list->head);
        }
        else
        {
            LOGD("Cannot write USB HID keyboard gadget, reseting message queue...");
            while(g_usbhid_message_keyboard_list->head != NULL)
                list_delete(g_usbhid_message_keyboard_list, g_usbhid_message_keyboard_list->head);
        }
        pthread_mutex_unlock(&g_usbhid_message_keyboard_mutex);
    }
    return NULL;
}

void *usbhid_message_consume_mouse_thread(void *dummy)
{
    usbhid_message_struct *message;
    int ret;

    (void)dummy; // unused variable for compiler warning

    while(still_run) {
        pthread_mutex_lock(&g_usbhid_message_mouse_mutex);
        while(g_usbhid_message_mouse_list->head == NULL)
            pthread_cond_wait(&g_usbhid_message_mouse_cond, &g_usbhid_message_mouse_mutex);
        message = (usbhid_message_struct *) g_usbhid_message_mouse_list->head->data;
        pthread_mutex_unlock(&g_usbhid_message_mouse_mutex);

        ret = usbhid_gadget_write_mouse(message->_dummy.mouse_report_buf);

        pthread_mutex_lock(&g_usbhid_message_mouse_mutex);
        if(ret == 0)
        {
            list_delete(g_usbhid_message_mouse_list, g_usbhid_message_mouse_list->head);
        }
        else
        {
            LOGD("Cannot write USB HID mouse gadget, reseting message queue...");
            while(g_usbhid_message_mouse_list->head != NULL)
                list_delete(g_usbhid_message_mouse_list, g_usbhid_message_mouse_list->head);
        }
        pthread_mutex_unlock(&g_usbhid_message_mouse_mutex);
    }
    return NULL;
}

void usbhid_message_consume_thread_init(void)
{
    still_run = 1;
    LOGI("Initalizing USB HID message consume thread...");
    usbhid_message_queue_init();
    pthread_create(&g_keyboard_thread, NULL, usbhid_message_consume_keyboard_thread, NULL);
    pthread_create(&g_mouse_thread, NULL,  usbhid_message_consume_mouse_thread, NULL);
    LOGI("USB HID message consume thread initalized");
}

void usbhid_message_consume_thread_destory(void)
{
    still_run = 0;
    LOGI("Stopping USB HID message consume thread...");
    pthread_join(g_keyboard_thread, NULL);
    pthread_join(g_mouse_thread, NULL);
    usbhid_message_queue_destory();
    LOGI("USB HID message consume thread stopped");
}
