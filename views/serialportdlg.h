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

    void fetch();
    void start();
    void read();
    void send();
};

#endif // SERIALPORTDLG_H
