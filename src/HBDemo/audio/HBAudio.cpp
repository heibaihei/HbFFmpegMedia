#include "HBAudio.h"

int audioGlobalInitial()
{
    av_register_all();
    avformat_network_init();
    
    return HB_OK;
}
