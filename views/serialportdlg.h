#ifndef SERIALPORTDLG_H
#define SERIALPORTDLG_H

#include <QDialog>

#include <QtSerialPort/QSerialPort>


namespace Ui {
class SerialPortDlg;
}

class SerialPortDlg : public QDialog
{
    Q_OBJECT

public:
    explicit SerialPortDlg(QWidget *parent = nullptr);
    ~SerialPortDlg();

private slots:
    void on_connBtn_clicked();

    void on_fetchBtn_clicked();

    void on_sendBtn_clicked();

private:
    Ui::SerialPortDlg *ui;

    QSerialPort *serial;

    // 连接状态，0：未连接；1：已连接
    int _status;
    // 当前连接的串口名
    QString _currentPort;

    // 获取串口列表
    void fetch();
    // 连接串口
    void start();
    // 读取串口数据
    void read();
    // 发送数据
    void send();
};

#endif // SERIALPORTDLG_H
