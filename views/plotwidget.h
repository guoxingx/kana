#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>

#include "views/qcustomplot.h"


namespace Ui {
class PlotWidget;
}

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = nullptr);
    ~PlotWidget();

    // 初始化热力图
    void init(int nx, int ny);
    // 更新数据
    void update(int x, int y, double value, bool refresh=true);
    // 主动刷新图表
    void refresh();

public slots:
    // 根据窗口尺寸变化
    void resizeEvent(QResizeEvent * re);

private:
    Ui::PlotWidget *ui;

    // 热力图
    QCPColorMap *_colorMap;
    QCPColorScale *_colorScale;
};

#endif // PLOTWIDGET_H
