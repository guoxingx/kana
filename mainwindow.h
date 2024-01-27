#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include "ui_mainwindow.h"
#include "camera/capturethread.h"
#include "views/modemainwindow.h"

#include <windows.h>
#include "CameraApi.h"

using namespace std;

typedef struct _WIDTH_HEIGHT{
    int     display_width;
    int     display_height;
    int     xOffsetFOV;
    int     yOffsetFOV;
    int     sensor_width;
    int     sensor_height;
    int     buffer_size;
}Width_Height;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

// 信号函数需要用 slots 来定义
private slots:
    // 接收相机图片信号的处理函数
    void slot_image_process(QImage img);

    void on_cameraConnBtn_clicked();

    void on_serialAct_triggered();

    void on_versionAct_triggered();

    void on_snapPathBtn_clicked();

    void on_snapBtn_clicked();

    void on_BPFAct_triggered();

    void on_bpBtn_clicked();

    void on_modeBtn_clicked();

private:
    Ui::MainWindow *ui;

    ModeMainWindow *_mode_window;
};
#endif // MAINWINDOW_H
