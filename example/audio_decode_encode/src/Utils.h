//
//  Utils.h
//  audioTranscode
//
//  Created by meitu on 2017/7/19.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef Utils_h
#define Utils_h

#include "AudioParam.h"

#define INNER_DEBUG_MODEL 0

static bool checkAudioParamValid(AudioParam_t *parm)
{
    if (parm == NULL) {
        return false;
    }
    
    if (parm->channels == 1 && av_sample_fmt_is_planar((AVSampleFormat)parm->sampleFmt)) {
        av_log(NULL, AV_LOG_WARNING, "if channel == 1 sample format cannot set %s\n", av_get_sample_fmt_name((AVSampleFormat)parm->sampleFmt));
        return false;
    }
    
    return true;
}


static int initOutputFrame(AVFrame **frame,
                           AudioParam_t *parm,
                           int samples)
{
    int ret;
    AVFrame *temp;
    
    if (frame == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Parmater err!\n");
        ret = AV_PARM_ERR;
        goto TAR_OUT;
        
    }
    temp = *frame;
    if (temp == NULL) {
        temp = av_frame_alloc();
        if (temp == NULL) {
            av_log(NULL, AV_LOG_ERROR, "Malloc frame err!\n");
            ret = AV_MALLOC_ERR;
            return ret;
        }
    }
    
    temp->nb_samples     = samples;
    temp->format         = parm->sampleFmt;
    temp->sample_rate    = parm->sampleRate;
    temp->channels       = parm->channels;
    temp->channel_layout = av_get_default_channel_layout(parm->channels);
    
    if ((ret = av_frame_get_buffer(temp, 0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Get frame buffer error![%s]\n", makeErrorStr(ret));
        return ret;
    }
    
    *frame = temp;
    
TAR_OUT:
    
    return ret;
}

#endif /* Utils_h */
