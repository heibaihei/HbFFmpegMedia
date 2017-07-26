/** 参考链接： http://blog.csdn.net/leixiaohua1020/article/details/29368911 
 *           http://www.360doc.com/content/15/0516/17/7821691_471031053.shtml
 */


#ifndef __HBFILTERWATER_H__
#define __HBFILTERWATER_H__

/**
 * 功能：将一张透明背景的PNG图片作为水印叠加到一个视频文件上；
 *      叠加工作是在解码后的YUV像素数据的基础上完成
 *      查看对应的媒体：./ffplay -f rawvideo -s 480x480 /Users/zj-db0519/Desktop/material/folder/video/100f2.yuv
 */

int HBFilterWaterTest(int argc, char **argv);


#endif
