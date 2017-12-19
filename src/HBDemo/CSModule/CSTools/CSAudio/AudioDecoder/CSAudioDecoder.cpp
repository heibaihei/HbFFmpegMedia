//
//  CSAudioDecoder.c
//  FFmpeg
//
//  Created by zj-db0519 on 2017/8/15.
//  Copyright © 2017年 meitu. All rights reserved.
//

#include "CSAudioDecoder.h"

namespace HBMedia {

static void EchoStatus(uint64_t status) {
    bool bReadEnd = ((status & DECODE_STATE_READPKT_END) != 0 ? true : false);
    bool bDecodeEnd = ((status & DECODE_STATE_DECODE_END) != 0 ? true : false);
    bool bReadAbort = ((status & DECODE_STATE_READPKT_ABORT) != 0 ? true : false);
    bool bDecodeAbort = ((status & DECODE_STATE_DECODE_ABORT) != 0 ? true : false);
    bool bFlushMode = ((status & DECODE_STATE_FLUSH_MODE) != 0 ? true : false);
    
    LOGI("[Work task: <Decoder>] Status: Read<End:%d, Abort:%d> | <Flush:%d> | Decode<End:%d, Abort:%d>", bReadEnd, bReadAbort, bFlushMode, bDecodeEnd, bDecodeAbort);
}

int CSAudioDecoder::S_MAX_BUFFER_CACHE = 8;
void* CSAudioDecoder::ThreadFunc_Audio_Decoder(void *arg) {
    return nullptr;
}

CSAudioDecoder::CSAudioDecoder() {
    memset(&mState, 0x00, sizeof(mState));
    audioParamInit(&mTargetAudioParams);
    audioParamInit(&mSrcAudioParams);
    
    mAudioStreamIndex = INVALID_STREAM_INDEX;
    mPInMediaFormatCtx = nullptr;
    
    mPInputAudioCodecCtx = nullptr;
    mPInputAudioCodec = nullptr;
    mPAudioResampleCtx = nullptr;
    mTargetFrameQueue = new FiFoQueue<AVFrame *>(S_MAX_BUFFER_CACHE);
    mTargetFrameQueueIPC = new ThreadIPCContext(0);
    mEmptyFrameQueueIPC = new ThreadIPCContext(S_MAX_BUFFER_CACHE);
    mDecodeThreadCtx.setFunction(nullptr, nullptr);
}

CSAudioDecoder::~CSAudioDecoder() {
}
    
int CSAudioDecoder::prepare() {
    return HB_OK;
}

int CSAudioDecoder::start() {
    return HB_OK;
}

int CSAudioDecoder::stop() {
    return HB_OK;
}

int CSAudioDecoder::release() {
    return HB_OK;
}

int CSAudioDecoder::receiveFrame(AVFrame **OutFrame) {
    return HB_OK;
}

int CSAudioDecoder::syncWait() {
    return HB_OK;
}

int CSAudioDecoder::_DecoderInitial() {
    return HB_OK;
}

int CSAudioDecoder::_ExportInitial() {
    return HB_OK;
}

int CSAudioDecoder::_ResampleInitial() {
    return HB_OK;
}

int CSAudioDecoder::_DoResample(AVFrame *pInFrame, AVFrame **pOutFrame) {
    return HB_OK;
}

int CSAudioDecoder::_DoExport(AVFrame **pOutFrame) {
    return HB_OK;
}

int CSAudioDecoder::_mediaParamInitial() {
    return HB_OK;
}

}
