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
    // 实例化相机数据流线程
    cm_thread = new CaptureThread(this);
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
    qDebug("receive image: %i, %i");

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

// 按钮动作 启动相机
void MainWindow::on_cameraConnBtn_clicked()
{
    // 初始化相机线程
    if (cm_thread->get_status() == -1)
    {
        int res = cm_thread->init();
        qDebug("camera init result: %d", res);

        // 连接信号
        connect(cm_thread, SIGNAL(captured(QImage)), this, SLOT(image_process(QImage)), Qt::BlockingQueuedConnection);
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
    QFileDialog* openFilePath = new QFileDialog( this, "Select Folder", "");     //打开一个目录选择对话框
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
    tSdkFrameHead	tFrameHead;
    BYTE			*pbyBuffer;
    BYTE			*pbImgBuffer;
    char            filename[512] = {0};
    QString path = ui->snapPathLine->text();

    char* dir = path.toLatin1().data();
    qDebug("save snap into path: %s", dir);

    // CameraSnapToBuffer抓拍一张图像保存到buffer中
    // !!!!!!注意：CameraSnapToBuffer 会切换分辨率拍照，速度较慢。做实时处理，请用CameraGetImageBuffer函数取图或者回调函数。
//    if(CameraSnapToBuffer(g_hCamera,&tFrameHead,&pbyBuffer,1000) == CAMERA_STATUS_SUCCESS)
//    {
//        switch(g_SaveImage_type){
//            case 1:
//            break;

//            case 2:
//            break;

//            case 3:
//                pbImgBuffer = (unsigned char*)malloc(g_tCapability.sResolutionRange.iHeightMax*g_tCapability.sResolutionRange.iWidthMax*3);
//                /*
//                将获得的相机原始输出图像数据进行处理，叠加饱和度、
//                颜色增益和校正、降噪等处理效果，最后得到RGB888
//                格式的图像数据。
//                */
//                CameraImageProcess(g_hCamera, pbyBuffer,pbImgBuffer,&tFrameHead);

//                //将图像缓冲区的数据保存成图片文件。
//                CameraSaveImage(g_hCamera, filename,pbImgBuffer, &tFrameHead, FILE_BMP, 100);
//                //释放由CameraGetImageBuffer获得的缓冲区。
//                CameraReleaseImageBuffer(g_hCamera,pbImgBuffer);
//                free(pbImgBuffer);
//            break;

//            case 4:
//                CameraSaveImage(g_hCamera, filename,pbyBuffer, &tFrameHead, FILE_RAW, 100);
//                CameraReleaseImageBuffer(g_hCamera,pbImgBuffer);
//            break;

//            default :
//            break;
//        }
//    }
}
