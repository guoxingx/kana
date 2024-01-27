#include "capturethread.h"


// 相机线程
// 对象初始化
CaptureThread::CaptureThread(QObject *parent) :
    QThread(parent)
{
//    msv = MinsVision();

    _status = 0;

    // 不知道为啥要组织一个灰度图数组
    for (int i = 0; i < 256; i++) {
        grayColourTable.append(qRgb(i, i, i));
    }
}

CaptureThread::~CaptureThread() {
    this->stop();
    quit();
    wait();
}

int CaptureThread::init() {
    msv.init(&sensorInfo);
    readBuf = (unsigned char*)malloc(sensorInfo.bufSizeMax * 3);
    _status = 0;
    return 0;
}

// 启动线程
// QThread 子类通过调用start() 启动线程，会调用子类重写的 run() 函数
void CaptureThread::run()
{
    _status = 1;

    // forever{} = for{} 即死循环
    forever
    {
        // quit状态，直接退出
        if (_status == -1) break;

        // pause状态，1s后继续循环
        if (_status == 2)
        {
            qDebug("thread pause...");
            // slepp(秒) msleep(毫秒) usleep(微秒)
            msleep(1000);
            continue;
        }

        // 获取图像数据
        int res = msv.get_image(readBuf);
        if (res != 0)
        {
            qDebug("failed to get image from MinsVision: %i", res);
            continue;
        }

        // 防止读取过程中quit状态改变 直接退出
        if(_status == -1) break;
        // 初始化QImage对象
        QImage img(readBuf, sensorInfo.width, sensorInfo.height, QImage::Format_Indexed8);
        img.setColorTable(grayColourTable);

        // 发送信号
        emit captured(img);

        // quit状态 直接退出
        if(_status == -1) break;
    }
}

// 启动传输
void CaptureThread::stream() { _status = 1; }

// 暂停传输
void CaptureThread::pause() { _status = 2; }

// 停止线程
void CaptureThread::stop() { _status = 0; }

// 返回当前状态
int8_t CaptureThread::get_status() { return _status; }

// 保存图片
int8_t CaptureThread::save_image(char* filename) {
//    CameraHandle    hCamera,
//    char*           lpszFileName,
//    BYTE*           pbyImageBuffer,
//    tSdkFrameHead*  pFrInfo,
//    UINT            byFileType,
//    BYTE            byQuality

    //将图像缓冲区的数据保存成图片文件。
//    CameraSaveImage(g_hCamera, filename, pbImgBuffer, &tFrameHead, FILE_BMP, 100);
    return 0;
}
