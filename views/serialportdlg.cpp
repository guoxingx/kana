#include "serialportdlg.h"
#include "ui_serialportdlg.h"

#include <QDateTime>
#include <QtSerialPort/QSerialPortInfo>


SerialPortDlg::SerialPortDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SerialPortDlg)
{
    ui->setupUi(this);

    this->fetch();
    this->serial = new QSerialPort;

    // 禁用发送按钮
    ui->sendBtn->setEnabled(false);
}

SerialPortDlg::~SerialPortDlg()
{
    delete ui;
}

// 获取串口信息
void SerialPortDlg::fetch()
{
    // 清除 portBox 已存在的选项
    int existed = ui->portBox->count();
    for (int i = 0; i < existed; i++)
    {
        ui->portBox->removeItem(0);
    }

    // 添加 portname 到 portBox 选项
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->portBox->addItem(info.portName());
    }
}

// 连接相机
void SerialPortDlg::start()
{
    // 串口名
    this->serial->setPortName(ui->portBox->currentText());
//    this->serial->setBaudRate(9600);
//    this->serial->setDataBits(QSerialPort::Data8);
//    this->serial->setParity(QSerialPort::NoParity);
//    this->serial->setStopBits(QSerialPort::OneStop);
//    this->serial->setFlowControl(QSerialPort::NoFlowControl);

    // ->是指针指向其成员的运算符。
    // . 是结构体的成员运算符。
    // ::是域作用符，是各种域性质的实体（比如类（不是对象）、名字空间等）调用其成员专用的。
    if (this->serial->open(QIODevice::ReadWrite))
    {
        _currentPort = ui->portBox->currentText();
        // 使用 " qDebug() << "一定要添加头文件 #include <QDebug>
        // 如果只使用括号，不需要引用
        // qString 不能直接打印
        // qPrintable() 将 QString 转化为可打印
        qDebug("open port %s", qPrintable(_currentPort));

        // 更改状态栏信息
        ui->statusLabel->setText(_currentPort.append("连接成功"));
        // 启用发送按钮
        ui->sendBtn->setEnabled(true);
        // 更新按钮文本
        ui->connBtn->setText("关闭");
        // 更新连接状态信息
        _status = 1;

        // 启动读取
        connect(this->serial, &QSerialPort::readyRead, this, &SerialPortDlg::read);

    } else {
        ui->statusLabel->setText(_currentPort.append("连接失败"));
        ui->sendBtn->setEnabled(false);
        qDebug("failed to open port: ", qPrintable(_currentPort));
    }
}

// 读取数据
void SerialPortDlg::read()
{
    // 读取当前全部数据
    QByteArray buf = this->serial->readAll();
    if (!buf.isEmpty())
    {
        // tr() 将 buf 转化成 QString
        QString text = tr(buf);

        // 添加到 textBrowser
        ui->textBrowser->append(text);

        // 获取当前时间戳（毫秒）
        qint64 tsm = QDateTime::currentMSecsSinceEpoch();
        qDebug("ts millisec: %d, text: %s", tsm, qPrintable(text));
    }
}

// 发送数据
void SerialPortDlg::send()
{
    // 将 textEdit 中的文本转为 byteArray 并发送
    qDebug("%s", qPrintable(ui->textEdit->toPlainText()));
    this->serial->write(ui->textEdit->toPlainText().toLatin1());
}

// 按钮动作：连接串口
void SerialPortDlg::on_connBtn_clicked()
{
    if (_status == 0) {
        // 直接调用函数 connect()
        this->start();
    } else {
        // 关闭串口
        _status = 0;
        _currentPort.clear();
        // 更改状态栏信息
        ui->statusLabel->setText("未连接");
        // 启用发送按钮
        ui->sendBtn->setEnabled(false);
        // 更新按钮文本
        ui->connBtn->setText("连接");
    }
}

// 按钮动作：刷新串口列表
void SerialPortDlg::on_fetchBtn_clicked()
{
    // 直接调用函数 fetch()
    this->fetch();
}

// 按钮动作：发送信息
void SerialPortDlg::on_sendBtn_clicked()
{
    // 直接调用函数 send()
    this->send();
}
