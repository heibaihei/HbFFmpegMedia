#ifndef __HB_SEPERATE_FORMAT_H__
#define __HB_SEPERATE_FORMAT_H__

/**
 * 参考链接： http://blog.csdn.net/leixiaohua1020/article/details/39802819
            http://blog.csdn.net/leixiaohua1020/article/details/39767055
 * 功能描述：视音频分离器（Demuxer）即是将封装格式数据（例如MKV）中的视频压缩数据（例如H.264）和音频压缩数据（例如AAC）分离开。如图所示。在这个过程中并不涉及到编码和解码。
 */

int HBSeperateFormat(int argc, char **argv);

#endif
