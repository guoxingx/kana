#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include <QImage>

#include "camera/types.h"
#include "camera/minsvision.h"


// 继承QThread，负责读取图片的昂贵操作
// 通过调用 对象的父类QThread 函数 start()来启动
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    // 我也不知道这行的具体意思...
    explicit CaptureThread(QObject *parent = 0);
    ~CaptureThread();
public:
    // 初始化配置
    int init();

    // QThread 子类实现 run() 以供父类调用
    void run();

    // 启动 暂停 退出
    void stream();
    void pause();
    void stop();

    // 返回相机状态
    int8_t get_status();

    // 保存图片
    int8_t save_image(char* filename);

private:
    // 相机状态
    // -1: 退出
    // 0: 未连接
    // 1: 启动
    // 2: 暂停
    int8_t _status;

//    // 退出状态
//    bool quit;
//    // 暂停状态
//    bool pause_status;

    // 显示数据buffer
    unsigned char* readBuf;
    Sensor_Info sensorInfo;

    // 灰度图使用
    QVector<QRgb> grayColourTable;


    MinsVision msv;

signals:
    void captured(QImage img);
};

#endif // CAPTURETHREAD_H
