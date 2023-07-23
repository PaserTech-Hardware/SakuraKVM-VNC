#include "vncserver_types.h"
#include "list.h"

int sakuravnc_clientdata_create(sakuravnc_clientdata *clientdata)
{
    int ret;
    ret = list_create(&clientdata->keysdown_list);
    if(ret)
        return ret;
    return 0;
}

int sakuravnc_clientdata_destroy(sakuravnc_clientdata *clientdata)
{
    int ret;
    ret = list_destroy(clientdata->keysdown_list);
    if(ret)
        return ret;
    return 0;
}
