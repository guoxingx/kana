#ifndef BEAMPROFILERDLG_H
#define BEAMPROFILERDLG_H

#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QGraphicsPixmapItem>

#include "camera/capturethread.h"


namespace Ui {
class BeamProfilerDlg;
}

class BeamProfilerDlg : public QDialog
{
    Q_OBJECT

public:
    explicit BeamProfilerDlg(QWidget *parent = nullptr);
    ~BeamProfilerDlg();

private slots:
    // 相机接收数据
    void slot_camera_image_process(QImage img);

    void on_processDirBtn_clicked();

    void on_workdirBtn_clicked();

    void on_cameraBtn_clicked();

    void on_captureBtn_clicked();

    void on_serialFetchBtn_clicked();

    void on_serialConnBtn_clicked();

private:
    Ui::BeamProfilerDlg *ui;

    // 串口
    QSerialPort *_serial;
    // 当前连接的串口名
    QString _currentPort;

    // 相机线程
    CaptureThread *_capture_thread;
    // 当前展示的图片
    QImage _current_img;
    // 处理Graphics
    QGraphicsScene *_current_scene;
    QGraphicsPixmapItem *_current_image_item;

    // 工作目录
    QString working_dir;

    // 打开串口连接
    int serial_connect();
    // 断开串口连接
    int serial_disconnect();
    // 读取串口数据
    void serial_read();
    // 获取串口列表
    int serial_fetch();
    // 发送串口数据
    int serial_send(const char *data);

    // 连接相机
    int camera_connect();
    // 断开相机
    int camera_disconnect();
    // 开始传输
    int camera_stream();
    // 暂停传输
    int camera_pause();
    // 相机采集图片
    int camera_capture();

    // 以下是model及其更新函数
    // 串口状态
    int _serial_status;
    void model_serial_status(int serial_status);
    // 相机状态
    int _camera_status;
    void model_camera_status(int camera_status);
};

// 功率/光谱/脉宽
#endif // BEAMPROFILERDLG_H
