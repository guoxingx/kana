#ifndef MINSVISION_H
#define MINSVISION_H

#include <windows.h>
#include "CameraApi.h"

#include "camera/types.h"

class MinsVision
{
public:
    MinsVision();

    int init(Sensor_Info* info);
    int get_image(unsigned char* res);

private:
//    int                  g_hCamera;          // 设备句柄
//    tSdkFrameHead        g_tFrameHead;       // 图像帧头信息
//    unsigned char        * g_pRawBuffer;     // raw数据
//    unsigned char        * g_pRgbBuffer;     // 处理后数据缓存区

//    Width_Height         g_W_H_INFO;         // 显示画板到大小和图像大小
//    BYTE                 * g_readBuf;        // 显示数据buffer
//    int                  g_read_fps;         // 统计帧率

//    tSdkCameraCapbility  g_pCameraInfo;   // 设备描述信息

    int                  hCamera;
    int                  bufSize;
    int                  bufSizeMax;
    tSdkCameraCapbility  pCameraInfo;    // 设备描述信息
    tSdkFrameHead        tFrameHead;     // 图像帧头信息
    unsigned char        * pRawBuf;      //raw数据
    unsigned char        * pRgbBuf;      //处理后数据缓存区
};

#endif // MINSVISION_H
