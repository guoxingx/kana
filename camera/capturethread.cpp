#include "capturethread.h"

#include "mainwindow.h"
#include <QDebug>

#include <windows.h>
#include "CameraApi.h"

// extern定义于 mainwindow.cpp
extern int                  g_hCamera;          //设备句柄
extern tSdkFrameHead        g_tFrameHead;       //图像帧头信息
extern unsigned char        * g_pRawBuffer;     //raw数据
extern unsigned char        * g_pRgbBuffer;     //处理后数据缓存区

extern Width_Height         g_W_H_INFO;         //显示画板到大小和图像大小
extern BYTE                 *g_readBuf;         //显示数据buffer
extern int                  g_read_fps;         //统计帧率

// 对象初始化
CaptureThread::CaptureThread(QObject *parent) :
    QThread(parent)
{
    pause_status = true;
    quit = false;

    // 不知道为啥要组织一个灰度图数组
    for (int i = 0; i < 256; i++) {
        grayColourTable.append(qRgb(i, i, i));
    }
}

// 启动线程
void CaptureThread::run()
{
    // forever{} = for{} 即死循环
    forever
    {
        // quit状态，直接退出
        if (quit) break;

        // pause状态，1s后继续循环
        if (pause_status) usleep(1000); continue;

        // CameraGetImageBuffer 获取相机数据
        //  params1：相机句柄；CameraHandle即int；
        //  params2：图像帧头信息；tSdkFrameHead*；
        //  params3: 返回数据bytes；BYTE**；
        //  params4：超时时间；毫秒；UINT；
        //  return： CAMERA_STATUS_XXX；相机状态

        if (CameraGetImageBuffer(g_hCamera, &g_tFrameHead, &g_pRawBuffer, 2000) == CAMERA_STATUS_SUCCESS)
        {
            // CameraImageProcess 处理相机数据，如饱和度，色彩，增益，降噪等
            //  params1：相机句柄；CameraHandle即int；
            //  params2：输入数据bytes；BYTE**；
            //  params3: 返回数据bytes；BYTE**；
            //  params4：图像帧头信息；tSdkFrameHead*；
            //  return： CAMERA_STATUS_XXX；相机状态；
            CameraImageProcess(g_hCamera, g_pRawBuffer, g_pRgbBuffer, &g_tFrameHead);

            // 释放缓存
            CameraReleaseImageBuffer(g_hCamera, g_pRawBuffer);

            // 用句柄判断照片类型
            if(g_tFrameHead.uiMediaType==CAMERA_MEDIA_TYPE_MONO8){
                // 单色图
                // 从rgbBuffer拷贝到readBuf，大小从WHInfo获取
                memcpy(g_readBuf, g_pRgbBuffer, g_W_H_INFO.buffer_size);

                // 防止读取过程中quit状态改变 直接退出
                if(quit) break;

                // 初始化QImage对象
                QImage img(g_readBuf, g_W_H_INFO.sensor_width, g_W_H_INFO.sensor_height, QImage::Format_Indexed8);
                img.setColorTable(grayColourTable);

                // 发送信号
                emit captured(img);

            } else {
                // 彩色图
                // 从rgbBuffer拷贝到readBuf，大小从WHInfo获取
                memcpy(g_readBuf, g_pRgbBuffer, g_W_H_INFO.buffer_size*3);

                // 防止读取过程中quit状态改变 直接退出
                if(quit) break;

                // 初始化QImage
                QImage img = QImage((const uchar*)g_readBuf, g_W_H_INFO.sensor_width, g_W_H_INFO.sensor_height, QImage::Format_RGB888);
                //QImage img(g_readBuf, g_W_H_INFO.sensor_width, g_W_H_INFO.sensor_height,QImage::Format_RGB888);

                // 发送信号
                emit captured(img);
            }

            // 统计抓取帧率
            g_read_fps++;

        }

        // quit状态 直接退出
        if(quit) break;
    }
}


// 启动传输
void CaptureThread::stream()
{
    pause_status = false;
}

// 暂停传输
void CaptureThread::pause()
{
    pause_status = true;
}

// 停止线程
void CaptureThread::stop()
{
    pause_status = false;
    quit = true;
}
