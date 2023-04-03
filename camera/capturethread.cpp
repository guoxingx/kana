#include "capturethread.h"


// 对象初始化
CaptureThread::CaptureThread(QObject *parent) :
    QThread(parent)
{
//    msv = MinsVision();

    status = -1;

    // 不知道为啥要组织一个灰度图数组
    for (int i = 0; i < 256; i++) {
        grayColourTable.append(qRgb(i, i, i));
    }
}

int CaptureThread::init()
{
//    msv.init(&sensorInfo);
//    readBuf = (unsigned char*)malloc(sensorInfo.bufSizeMax * 3);
    return 0;
}

// 启动线程
void CaptureThread::run()
{
    status = 1;

    // forever{} = for{} 即死循环
    forever
    {
        // quit状态，直接退出
        if (status == -1) break;

        // pause状态，1s后继续循环
        if (status == 0)
        {
            qDebug("thread pause...");
            // slepp(秒) msleep(毫秒) usleep(微秒)
            msleep(1000);
            continue;
        }

        // 获取图像数据
//        msv.get_image(readBuf);

        // 防止读取过程中quit状态改变 直接退出
        if(status == -1) break;
        // 初始化QImage对象
        QImage img(readBuf, sensorInfo.width, sensorInfo.height, QImage::Format_Indexed8);
        img.setColorTable(grayColourTable);

        // 发送信号
        emit captured(img);

        // quit状态 直接退出
        if(status == -1) break;
    }
}

// 启动传输
void CaptureThread::stream()
{
    status = 1;
}

// 暂停传输
void CaptureThread::pause()
{
    status = 0;
}

// 停止线程
void CaptureThread::stop()
{
    status = -1;
}

int8_t CaptureThread::get_status()
{
    return status;
}
