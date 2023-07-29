#include <stdio.h>
#include "logging.h"
#include "vncserver.h"

int main(int argc, char *argv[])
{
    int ret;
    ret = logging_init(LOG_LEVEL_INFO);
    if(ret)
        return ret;
    
    vncserver_init(argc, argv);
    
    logging_close();
    return 0;
}
