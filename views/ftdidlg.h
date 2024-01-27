#ifndef FTDIDLG_H
#define FTDIDLG_H

#include <QDialog>

namespace Ui {
class FtdiDlg;
}

class FtdiDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FtdiDlg(QWidget *parent = nullptr);
    ~FtdiDlg();

private:
    Ui::FtdiDlg *ui;
};

#endif // FTDIDLG_H
