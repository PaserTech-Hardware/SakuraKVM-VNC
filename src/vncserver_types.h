#ifndef __SAKURAKVMD_VNCSERVER_TYPES__
#define __SAKURAKVMD_VNCSERVER_TYPES__

#include "list.h"
#include "usbhid_gadget.h"

typedef struct _sakuravnc_clientdata {
    list_ctx_t *keysdown_list;
    unsigned char keyboardReport[KEY_REPORT_LENGTH];
} sakuravnc_clientdata, *p_sakuravnc_clientdata;

int sakuravnc_clientdata_create(sakuravnc_clientdata *clientdata);
int sakuravnc_clientdata_destroy(sakuravnc_clientdata *clientdata);

#endif
