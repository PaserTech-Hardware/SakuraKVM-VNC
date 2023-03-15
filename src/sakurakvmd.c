#include <stdio.h>
#include "logging.h"

int main()
{
    int ret;
    ret = logging_init(LOG_LEVEL_VERBOSE);
    if(ret)
        return ret;
    
    // do something here
    
    logging_close();
    return 0;
}
