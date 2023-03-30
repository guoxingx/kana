#include "mainwindow.h"

#include "views/versiondlg.h"
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include "camera/capturethread.h"
#include <stdio.h>

#ifdef _WIN64
#pragma comment(lib,"MVCAMSDK_X64.lib")
#else
#pragma comment(lib,"MVCAMSDK.lib")
#endif

int                     g_hCamera = -1;     //设备句柄
unsigned char           * g_pRawBuffer=NULL;     //raw数据
unsigned char           * g_pRgbBuffer=NULL;     //处理后数据缓存区
tSdkFrameHead           g_tFrameHead;       //图像帧头信息
tSdkCameraCapbility     g_tCapability;      //设备描述信息

Width_Height            g_W_H_INFO;         //显示画板到大小和图像大小
BYTE                    *g_readBuf=NULL;    //画板显示数据区
int                     g_read_fps=0;       //统计读取帧率
int                     g_disply_fps=0;     //统计显示帧率

// 初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), cm_scene(0), cm_image_item(0)
{
    ui->setupUi(this);

    // 这里要防止中文乱码的代码
    ui->statusbar->showMessage(tr("寄"), 3000);

    // 设置相机图片UI
    cm_scene = new QGraphicsScene(this);
    ui->cameraGV->setScene(cm_scene);
}

// 结束 销毁
MainWindow::~MainWindow()
{
    delete ui;
}

// 启动相机
void MainWindow::startCamera()
{
    // 初始化相机数据流线程
    cm_thread = new CaptureThread(this);
    // 连接信号
//    connect(m_thread, SIGNAL(captured(QImage)), this, SLOT(Image_process(QImage)), Qt::BlockingQueuedConnection);
}

// 收到图片更新信号 处理图片
void MainWindow::Image_process(QImage img)
{
    // 若相机线程quit状态 直接退出
    if (cm_thread->quit) return;

    // 清除当前图片
    if (cm_image_item)
    {
        cm_scene->removeItem(cm_image_item);
        delete cm_image_item;
        cm_image_item = 0;
    }

    // 展示图片
    cm_image_item = cm_scene->addPixmap(QPixmap::fromImage(img));
    cm_scene->setSceneRect(0, 0, img.width(), img.height());

    // 更新展示fps
    g_disply_fps++;
}

// 按钮动作 显示版本
void MainWindow::on_actionversion_triggered()
{
    // 启动版本信息窗口
    VersionDlg dlg;
    dlg.exec();
}

// 按钮动作 启动相机
void MainWindow::on_cameraConnBtn_clicked()
{
    cm_thread->run();
    cm_thread->stream();
}

