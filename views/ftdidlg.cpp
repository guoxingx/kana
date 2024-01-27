#include "ftdidlg.h"
#include "ui_ftdidlg.h"


// 线阵探测器 调试页面
FtdiDlg::FtdiDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FtdiDlg)
{
    ui->setupUi(this);
}

FtdiDlg::~FtdiDlg()
{
    delete ui;
}
