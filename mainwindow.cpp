#include "mainwindow.h"
#include "qdebug.h"

#include "views/versiondlg.h"
#include "views/serialportdlg.h"
#include "views/beamprofilerdlg.h"
#include "camera/capturethread.h"
#include "analyse/measure.h"

#include <stdio.h>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QMessageBox>


// #include <> 的查找位置是标准库头文件所在目录
// #include "" 的查找位置是当前源文件所在目录

#ifdef _WIN64
    #pragma comment(lib,"MVCAMSDK_X64.lib")
#else
    #pragma comment(lib,"MVCAMSDK.lib")
#endif

// int                     g_hCamera = -1;     //设备句柄
//unsigned char           * g_pRawBuffer=NULL;     //raw数据
//unsigned char           * g_pRgbBuffer=NULL;     //处理后数据缓存区
//tSdkFrameHead           g_tFrameHead;       //图像帧头信息
//tSdkCameraCapbility     g_pCameraInfo;      //设备描述信息

//Width_Height            g_W_H_INFO;         //显示画板到大小和图像大小
//BYTE                    *g_readBuf=NULL;    //画板显示数据区
//int                     g_read_fps=0;       //统计读取帧率

//统计显示帧率
int g_disply_fps=0;

// 当前图片
QImage current_img;

// 图片序号
int gImageIndex = 0;

// 相机数据流处理线程
CaptureThread *capture_thread;

// 处理Graphics
QGraphicsScene *current_scene;
QGraphicsPixmapItem *current_image_item;


// 初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 这里要防止中文乱码的代码
    ui->statusbar->showMessage(tr("寄"), 3000);

    _mode_window = new ModeMainWindow();

    // 设置相机图片UI
    // QGraphicsView 用 setScene(QGraphicsScene&) 绑定一个 Scene 对象
//    current_scene = new QGraphicsScene(this);
//    ui->cameraGV->setScene(current_scene);

    // 实例化相机数据流线程
    capture_thread = new CaptureThread(this);

//    Measure m = Measure();
//    int res = m.test();
//    qDebug("beamwidth test: %d", res);
}

// 结束 销毁
MainWindow::~MainWindow()
{
    // 在销毁 mainwindow 之前终止相机线程
    capture_thread->pause();
    capture_thread->stop();
    capture_thread->exit(0);
    delete capture_thread;

    // 释放模式窗口
    _mode_window->close();
    delete _mode_window;
//    delete current_scene;

    delete ui;
}

// 收到图片更新信号 处理图片
void MainWindow::slot_image_process(QImage img)
{
    // 清除当前图片
    if (current_image_item) {
//        current_scene->removeItem(current_image_item);
        delete current_image_item;
        current_image_item = nullptr;
    }

    // 展示图片
//    current_image_item = current_scene->addPixmap(QPixmap::fromImage(img));
//    current_scene->setSceneRect(0, 0, img.width(), img.height());

    // 替换当前图片
    current_img = img;

    // 更新展示fps
    g_disply_fps++;
}

// 按钮动作 启动相机
void MainWindow::on_cameraConnBtn_clicked()
{
    // 初始化相机线程
    if (capture_thread->get_status() == -1)
    {
        int res = capture_thread->init();
        qDebug("camera init result: %d", res);

        // 连接信号
        connect(capture_thread, SIGNAL(captured(QImage)), this,
                SLOT(slot_image_process(QImage)), Qt::BlockingQueuedConnection);

        // QThread 子类通过调用start() 启动线程，会调用子类重写的 run() 函数
        capture_thread->start();
        qDebug("start camera");
    }

    // 启动相机
    if (capture_thread->get_status() == 0) {
        capture_thread->stream();
//        ui->cameraConnBtn->setText("暂停相机");

    } else if (capture_thread->get_status() == 1) {
        capture_thread->pause();
//        ui->cameraConnBtn->setText("打开相机");
    }
    qDebug("camera status: %i", capture_thread->get_status());
}

// 工具栏按钮动作 打开串口页面
void MainWindow::on_serialAct_triggered()
{
    // 初始化串口页面
    SerialPortDlg dlg;
    if (dlg.exec() == QDialog::Accepted)
        // 页面结束，接受
        qDebug("serial debug dlg: accept");
    else
        // 页面结束，拒绝 or 关闭
        qDebug("serial debug dlg: rejected");
}

// 按钮动作 显示版本
void MainWindow::on_versionAct_triggered() { VersionDlg dlg; dlg.exec(); }

// 选择路径：相机图片保存路径
void MainWindow::on_snapPathBtn_clicked()
{
    //打开一个目录选择对话框
    QFileDialog* openFilePath = new QFileDialog( this, "Select Folder", "");
    openFilePath-> setFileMode( QFileDialog::Directory );
    if ( openFilePath->exec() == QDialog::Accepted ) {
        QStringList paths = openFilePath->selectedFiles();
//        ui->snapPathLine->setText(paths[0]);
    }
    delete openFilePath;
}

// 保存图片：相机当前帧
void MainWindow::on_snapBtn_clicked()
{
    if (capture_thread->get_status() != 1)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("操作提示");
        msgBox.setText("相机未启动！");
        msgBox.exec();
        return;
    }

//    // 获取保存路径
//    QString path = ui->snapPathLine->text();
//    if (path.length() == 0)
//    {
//        QMessageBox msgBox;
//        msgBox.setWindowTitle("操作提示");
//        msgBox.setText("未选取路径！");
//        msgBox.exec();
//        return;
//    }
//    gImageIndex ++;

//    // 添加文件名
//    path.append("/" + QString::number(gImageIndex) + ".bmp");
//    qDebug("image: %p", &current_img);

//    // 保存图片
//    bool res = current_img.save(path);
//    if (!res) {
//        qDebug("failed to save image: %s", path.toStdString().data());
//        QMessageBox msgBox;
//        msgBox.setWindowTitle("操作提示");
//        msgBox.setText("图片保存失败！");
//        msgBox.exec();
//    } else {
//        ui->statusbar->showMessage(tr("图片保存成功"), 6000);
//    }
}

// 光束质量界面
void MainWindow::on_BPFAct_triggered()
{
    // 初始化串口页面
    BeamProfilerDlg dlg;
    if (dlg.exec() == QDialog::Accepted)
        // 页面结束，接受
        qDebug("BeamProfilerDlg: accept");
    else
        // 页面结束，拒绝 or 关闭
        qDebug("BeamProfilerDlg: rejected");
}


void MainWindow::on_bpBtn_clicked()
{
    on_BPFAct_triggered();
}

// 按钮动作 启动模式页面
void MainWindow::on_modeBtn_clicked()
{
    if (_mode_window->isHidden())
        _mode_window->show();
    else
        _mode_window->hide();
}

