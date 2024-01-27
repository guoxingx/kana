#include "beamprofilerdlg.h"
#include "ui_beamprofilerdlg.h"

#include <QDateTime>
#include <QtSerialPort/QSerialPortInfo>
#include <QFileDialog>
#include <QMessageBox>

#include "analyse/measure.h"


BeamProfilerDlg::BeamProfilerDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BeamProfilerDlg)
{
    ui->setupUi(this);

    // 初始化model
    _serial_status = 0;
    _camera_status = 0;

    // 初始化串口
    _serial = new QSerialPort;

    // 设置相机图片UI
    // QGraphicsView 用 setScene(QGraphicsScene&) 绑定一个 Scene 对象
    _current_scene = new QGraphicsScene(this);
    ui->cameraGV->setScene(_current_scene);
    _current_image_item = new QGraphicsPixmapItem();
    // 实例化相机数据流线程
    _capture_thread = new CaptureThread(this);
    // 自动连接相机
    int res = this->camera_connect();
    if (res) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("操作提示");
        msgBox.setText("相机启动失败，请检查连接！");
        msgBox.exec();
        return;
    } else {
        this->camera_stream();
    }
}

BeamProfilerDlg::~BeamProfilerDlg() {
    delete ui;

    // 释放相机线程
    delete _capture_thread;

    // 释放图片展示框
    delete _current_image_item;
    delete _current_scene;

    // 释放串口连接
    _serial->close();
    delete _serial;
}

// 获取串口信息
int BeamProfilerDlg::serial_fetch() {
    // 清除 portBox 已存在的选项
    int existed = ui->serialportBox->count();
    for (int i = 0; i < existed; i++)
        ui->serialportBox->removeItem(0);

    // 添加 portname 到 portBox 选项
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        ui->serialportBox->addItem(info.portName());

    return 0;
}

// 串口连接
int BeamProfilerDlg::serial_connect() {
    // 串口名
    QString portText = ui->serialportBox->currentText();
    if (portText.isNull() || portText.isEmpty()) { return -1; }
    _serial->setPortName(portText);

    // ->是指针指向其成员的运算符。
    // . 是结构体的成员运算符。
    // ::是域作用符，是各种域性质的实体（比如类（不是对象）、名字空间等）调用其成员专用的。
    if (_serial->open(QIODevice::ReadWrite)) {
        _currentPort = ui->serialportBox->currentText();
        // 使用 " qDebug() << "一定要添加头文件 #include <QDebug>
        // 如果只使用括号，不需要引用
        // qString 不能直接打印
        // qPrintable() 将 QString 转化为可打印
        qDebug("open port %s", qPrintable(_currentPort));

        // 更新model
        this->model_serial_status(1);

        // 启动读取
        connect(_serial, &QSerialPort::readyRead, this, &BeamProfilerDlg::serial_read);
        return 0;

    } else {
        // 更新model
        this->model_serial_status(-1);
        return 1;
    }
}

// 串口断连
int BeamProfilerDlg::serial_disconnect() {
    // 关闭串口
    _serial->close();
    _currentPort.clear();

    // 更新model
    this->model_serial_status(0);
    return 0;
}

// 读取数据
void BeamProfilerDlg::serial_read()
{
    // 读取当前全部数据
    QByteArray buf = _serial->readAll();
    if (!buf.isEmpty()) {
        // tr() 将 buf 转化成 QString
        QString text = tr(buf);

        // 获取当前时间戳（毫秒）
        qint64 tsm = QDateTime::currentMSecsSinceEpoch();
        qDebug("ts millisec: %d, text: %s", tsm, qPrintable(text));
    }
}

// 连接相机
int BeamProfilerDlg::camera_connect() {
    qDebug("camera_connect()");
    // 初始化相机线程
    if (_capture_thread->get_status() != 0) { return 1; }

    int res = _capture_thread->init();
    qDebug("camera init result: %d", res);

    // 连接信号
    connect(_capture_thread, SIGNAL(captured(QImage)), this,
            SLOT(slot_camera_image_process(QImage)), Qt::BlockingQueuedConnection);

    // QThread 子类通过调用start() 启动线程，会调用子类重写的 run() 函数
    _capture_thread->start();
    qDebug("start camera");

    int8_t status = _capture_thread->get_status();
    model_camera_status(status);
    qDebug("camera status: %i", status);
    return 0;
}

// 断开相机
int BeamProfilerDlg::camera_disconnect() { _capture_thread->stop(); return 0; }

// 相机采集图片
int BeamProfilerDlg::camera_capture() { return -1; }

// 开始传输
int BeamProfilerDlg::camera_stream() {
    _capture_thread->stream();
    model_camera_status(_capture_thread->get_status());
    return 0;
}

// 暂停传输
int BeamProfilerDlg::camera_pause() {
    _capture_thread->pause();
    model_camera_status(_capture_thread->get_status());
    return 0;
}

// 相机 处理图片
void BeamProfilerDlg::slot_camera_image_process(QImage img) {
    // 清除当前图片
    if (_current_image_item) {
        _current_scene->removeItem(_current_image_item);
        delete _current_image_item;
        _current_image_item = nullptr;
    }

    // 展示图片
    _current_image_item = _current_scene->addPixmap(QPixmap::fromImage(img));
    _current_scene->setSceneRect(0, 0, img.width(), img.height());

    // 替换当前图片
    _current_img = img;
}


// 发送数据
int BeamProfilerDlg::serial_send(const char *data) { return _serial->write(data); }

// model函数 串口状态变化
void BeamProfilerDlg::model_serial_status(int serial_status) {
    if (serial_status == _serial_status) { return; }
    qDebug("model update: serial status: %d", serial_status);
    _serial_status = serial_status;
    QString color, buttonText, statusText;
    statusText = "位移平台：";
    if (serial_status == 1) {
        statusText = statusText.append(_currentPort).append("连接成功");
        color = "color: rgb(0, 170, 0)";
        buttonText = "断开平移台";
    } else if (serial_status == 0) {
        statusText = statusText.append("未连接");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
    } else if (serial_status == -1) {
        statusText = statusText.append(_currentPort).append("连接失败");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
        qDebug("serial conn failed: ", qPrintable(_currentPort));

    } else {
        statusText = statusText.append(_currentPort).append(QString("状态错误%d").arg(serial_status));
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
        qDebug("serial status error：%d", serial_status);
    }
    ui->serialStatusLabel->setText(statusText);
    ui->serialStatusLabel->setStyleSheet(color);
    ui->serialConnBtn->setText(buttonText);
}

// model函数 相机状态变化
void BeamProfilerDlg::model_camera_status(int camera_status) {
    if (camera_status == _camera_status) { return; }
    qDebug("model update: camera status: %d", camera_status);
    _camera_status = camera_status;
    QString statusText, color, buttonText;
    statusText = "相机状态：";
    if (camera_status == 1) {
        statusText = statusText.append("传输中");
        color = "color: rgb(0, 170, 0)";
        buttonText = "暂停传输";
    } else if (camera_status == 2) {
        statusText = statusText.append("暂停");
        color = "color: rgb(170, 120, 0)";
        buttonText = "开始传输";
    } else if (camera_status == 0) {
        statusText = statusText.append("断开连接");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接相机";
    } else {
        statusText = statusText.append("错误");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接相机";
    }
    ui->cameraStatusLabel->setText(statusText);
    ui->cameraStatusLabel->setStyleSheet(color);
    ui->cameraBtn->setText(buttonText);
}

// 按钮动作 选择图片文件夹进行处理，生成待拟合数据（位置-束宽）
void BeamProfilerDlg::on_processDirBtn_clicked()
{
    // 图片目录
    QString dirpath;
    QFileDialog* openFilePath = new QFileDialog( this, "Select Folder", "");
    openFilePath-> setFileMode( QFileDialog::Directory );
    if ( openFilePath->exec() == QDialog::Accepted )
        dirpath = openFilePath->selectedFiles()[0];

    if (dirpath.isEmpty())
        return;

    // filedlg选择的目录，默认目录一定存在
    QDir dir(dirpath);
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.tif" << "*.png" << "*.jpg" << "*.jpeg";
    dir.setNameFilters(filters);
    QStringList imageNameList = dir.entryList();
    // 目录下没有所指定格式的图片文件，提示错误并退出函数
    if (imageNameList.count() <= 0) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("操作提示");
        msgBox.setText("该目录下没有可用图片！");
        msgBox.exec();
        return;
    } else {
        QMessageBox msgBox;
        msgBox.setWindowTitle("操作提示");
        msgBox.setText(QString("%1张可用图片，点击开始计算M2因子").arg(imageNameList.count()));
        msgBox.exec();
    }

    // 循环目录下所有图片，计算其束宽
    for (int i = 0; i < imageNameList.count(); i++) {
        Measure m = Measure();
        // 若用qstring.append()会覆盖原变量
        m.imagepath = (dirpath + "/" + imageNameList[i]).toStdString();
        int res = m.cal_beamwidth();
        qDebug("res: %d, path: %s, beamwidth: %f", res,
               qPrintable((dirpath + "/" + imageNameList[i])), m.beamwidth);
    }

    // 添加时间戳信息


    // 添加位置信息


    // 计算M2因子

}

// 按钮动作 选择工作目录
void BeamProfilerDlg::on_workdirBtn_clicked()
{
    //打开一个目录选择对话框
    QFileDialog* openFilePath = new QFileDialog( this, "Select Folder", "");
    openFilePath-> setFileMode( QFileDialog::Directory );
    if ( openFilePath->exec() == QDialog::Accepted ) {
        QStringList paths = openFilePath->selectedFiles();
        ui->snapPathLabel->setText(paths[0]);
        this->working_dir = paths[0];
    }
    delete openFilePath;
}

// 按钮动作 打开 / 关闭相机
void BeamProfilerDlg::on_cameraBtn_clicked()
{
    if (_camera_status == 0) {
        this->camera_connect();
    } else {

    }
}

// 按钮动作 拍摄相片
void BeamProfilerDlg::on_captureBtn_clicked()
{
    if (_capture_thread->get_status() != 1)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("操作提示");
        msgBox.setText("相机未启动！");
        msgBox.exec();
        return;
    }
}

// 按钮动作  刷新串口列表
void BeamProfilerDlg::on_serialFetchBtn_clicked() { this->serial_fetch(); }

// 按钮动作  连接 / 关闭 串口
void BeamProfilerDlg::on_serialConnBtn_clicked()
{
    if (_serial_status == 0) {
        this->serial_connect();
    } else {
        this->serial_disconnect();
    }
}
