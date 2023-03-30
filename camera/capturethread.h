#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include <QImage>

// 继承QThread，负责读取图片的昂贵操作
class CaptureThread : public QThread
{
    Q_OBJECT
public:
    // 我也不知道这行的具体意思...
    explicit CaptureThread(QObject *parent = 0);

public:
    // 启动 恢复 暂停 退出
    void run();
    void stream();
    void pause();
    void stop();

    // 退出状态，可以由外部获取
    bool quit;

private:
    // 暂停状态
    bool pause_status;

    // 灰度图使用
    QVector<QRgb> grayColourTable;

signals:
    void captured(QImage img);
};

#endif // CAPTURETHREAD_H
