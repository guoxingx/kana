#include "mainwindow.h"
#include "qdebug.h"

#include "views/versiondlg.h"
#include "views/serialportdlg.h"
#include "camera/capturethread.h"

#include <stdio.h>
#include <QFileDialog>
#include <QGraphicsPixmapItem>

// #include <> 的查找位置是标准库头文件所在目录
// #include "" 的查找位置是当前源文件所在目录

#ifdef _WIN64
#pragma comment(lib,"MVCAMSDK_X64.lib")
#else
#pragma comment(lib,"MVCAMSDK.lib")
#endif

int                     g_hCamera = -1;     //设备句柄
//unsigned char           * g_pRawBuffer=NULL;     //raw数据
unsigned char           * g_pRgbBuffer=NULL;     //处理后数据缓存区
//tSdkFrameHead           g_tFrameHead;       //图像帧头信息
//tSdkCameraCapbility     g_pCameraInfo;      //设备描述信息

Width_Height            g_W_H_INFO;         //显示画板到大小和图像大小
BYTE                    *g_readBuf=NULL;    //画板显示数据区
int                     g_read_fps=0;       //统计读取帧率
int                     g_disply_fps=0;     //统计显示帧率

int gImageIndex = 0;

// 初始化
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), cm_scene(0), cm_image_item(0)
{
    ui->setupUi(this);

    // 这里要防止中文乱码的代码
    ui->statusbar->showMessage(tr("寄"), 3000);

    // 设置相机图片UI
    // QGraphicsView 用 setScene(QGraphicsScene&) 绑定一个 Scene 对象
    cm_scene = new QGraphicsScene(this);
    ui->cameraGV->setScene(cm_scene);
    // 实例化相机数据流线程
    cm_thread = new CaptureThread(this);

//    current_img = nullptr;
}

// 结束 销毁
MainWindow::~MainWindow()
{
    // 在销毁 mainwindow 之前终止相机线程
    cm_thread->pause();
    cm_thread->stop();
    cm_thread->exit(0);

    delete ui;
}

// 收到图片更新信号 处理图片
void MainWindow::image_process(QImage img)
{
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

    // 替换当前图片
    current_img = img;

    // 更新展示fps
    g_disply_fps++;
}

// 按钮动作 启动相机
void MainWindow::on_cameraConnBtn_clicked()
{
    // 初始化相机线程
    if (cm_thread->get_status() == -1)
    {
        int res = cm_thread->init();
        qDebug("camera init result: %d", res);

        // 连接信号
        connect(cm_thread, SIGNAL(captured(QImage)), this,
                SLOT(image_process(QImage)), Qt::BlockingQueuedConnection);
        cm_thread->start();
        qDebug("start camera");
    }

    // 启动相机
    if (cm_thread->get_status() == 0)
    {
        this->cm_thread->stream();
        ui->cameraConnBtn->setText("暂停相机");
    }
    // 暂停相机
    else if (cm_thread->get_status() == 1)
    {
        this->cm_thread->pause();
        ui->cameraConnBtn->setText("打开相机");
    }
    qDebug("camera status: %i", cm_thread->get_status());
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
void MainWindow::on_versionAct_triggered()
{
    // 启动版本信息窗口
    VersionDlg dlg;
    dlg.exec();
}

// 选择路径：相机图片保存路径
void MainWindow::on_snapPathBtn_clicked()
{
    //打开一个目录选择对话框
    QFileDialog* openFilePath = new QFileDialog( this, "Select Folder", "");
    openFilePath-> setFileMode( QFileDialog::Directory );
    if ( openFilePath->exec() == QDialog::Accepted )
    {
        QStringList paths = openFilePath->selectedFiles();
        ui->snapPathLine->setText(paths[0]);
    }
    delete openFilePath;
}

// 保存图片：相机当前帧
void MainWindow::on_snapBtn_clicked()
{
    if (cm_thread->get_status() != 1)
    {
        ui->statusbar->showMessage(tr("相机未启动"), 6000);
        return;
    }

    // 获取保存路径
    QString path = ui->snapPathLine->text();
    if (path.length() == 0)
    {
        ui->statusbar->showMessage(tr("未选取路径"), 6000);
        return;
    }
    gImageIndex ++;

    // 添加文件名
    path.append("/" + QString::number(gImageIndex) + ".bmp");
    qDebug("image: %p", &current_img);

    // 保存图片
    bool res = current_img.save(path);
    if (!res) {
        qDebug("failed to save image: %s", path.toStdString().data());
        ui->statusbar->showMessage(tr("图片保存失败"), 6000);
    } else {
        ui->statusbar->showMessage(tr("图片保存成功"), 6000);
    }
}
