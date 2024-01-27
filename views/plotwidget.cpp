#include "plotwidget.h"
#include "ui_plotwidget.h"


PlotWidget::PlotWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotWidget)
{
    ui->setupUi(this);

    // clear default axis rect so we can start from scratch
//    ui->customPlot->plotLayout()->clear();

//    QCPAxisRect *wideAxisRect = new QCPAxisRect(ui->customPlot);
//    wideAxisRect->setupFullAxesBox(true);
//    wideAxisRect->axis(QCPAxis::atRight, 0)->setTickLabels(true);

    // add an extra axis on the left and color its numbers
//    wideAxisRect->addAxis(QCPAxis::atLeft)->setTickLabelColor(QColor("#6050F8"));
    // insert axis rect in first row
//    ui->customPlot->plotLayout()->addElement(0, 0, wideAxisRect);

    // 设置可以拖拽 / 缩放
    ui->customPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    ui->customPlot->axisRect()->setupFullAxesBox(true);
    ui->customPlot->xAxis->setLabel("x");
    ui->customPlot->yAxis->setLabel("y");

    // 初始化热力图
//    _colorMap = new QCPColorMap(wideAxisRect->axis(QCPAxis::atBottom), wideAxisRect->axis(QCPAxis::atLeft));
    _colorMap = new QCPColorMap(ui->customPlot->xAxis, ui->customPlot->yAxis);

    // 添加颜色参照表
    _colorScale = new QCPColorScale(ui->customPlot);
    // 添加到右侧
    ui->customPlot->plotLayout()->addElement(0, 1, _colorScale);
    _colorScale->setType(QCPAxis::atRight);
    // 与热力图建立关联
    _colorMap->setColorScale(_colorScale);
    _colorScale->axis()->setLabel("光强度");

    // 设置热力图颜色渐变
    _colorMap->setGradient(QCPColorGradient::gpPolar);

    // 图表与参照表的布局关系
    QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->customPlot);
    ui->customPlot->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
    _colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);

    ui->customPlot->rescaleAxes();
}

PlotWidget::~PlotWidget()
{
    delete ui;
}

void PlotWidget::resizeEvent(QResizeEvent * re) {
    ui->customPlot->resize(this->width(), this->height());
}

// 初始化热力图
void PlotWidget::init(int nx, int ny) {
    _colorMap->data()->clear();
    // we want the color map to have nx * ny data points
    _colorMap->data()->setSize(nx, ny);
    // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
    _colorMap->data()->setRange(QCPRange(0, nx), QCPRange(0, ny));
    // now we assign some data, by accessing the QCPColorMapData instance of the color map:

    // 根据现有数据 rescale
    ui->customPlot->rescaleAxes();
}

// 更新数据
void PlotWidget::update(int x, int y, double value, bool refresh) {
    _colorMap->data()->setCell(x, y, value);
    if (refresh) {
        _colorMap->rescaleDataRange();
        ui->customPlot->rescaleAxes();
    }
}

void PlotWidget::refresh() {
    _colorMap->rescaleDataRange();
    ui->customPlot->rescaleAxes();
}
