#include "versiondlg.h"
#include "ui_versiondlg.h"

VersionDlg::VersionDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionDlg)
{
    ui->setupUi(this);
}

VersionDlg::~VersionDlg()
{
    delete ui;
}
