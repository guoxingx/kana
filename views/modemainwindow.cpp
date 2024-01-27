#include "modemainwindow.h"
#include "ui_modemainwindow.h"

#include <QSerialPortInfo>
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>


//DataWorker::DataWorker(QObject *parent) : QObject(parent) {}

// 初始化波段
void DataWorker::slot_data_init(QList<float> wavelength, int start, int end, QString dir) {
    _wavelength = wavelength;
    _wavelength_range.first = start;
    _wavelength_range.second = end;
    _datalist.clear();
    _dir = dir;
    _valid_data_indexes.clear();
}

// 更新数据，index=-1表示添加到末尾，否则替换对应index位置的数据
void DataWorker::slot_data_update(QVariant var, int index) {
    // 添加数据
    if (index == -1)
    {
        ScanData data = var.value<ScanData>();
        _datalist.append(data);

        // 记录更新index后第一条数据的位置
        if (data.latestindex > _valid_data_indexes.size() - 1)
        {
            // 记录datalist中对应的位置
            qDebug("save valid index for %d at %d", data.latestindex, _datalist.size() - 1);
            _valid_data_indexes.append(_datalist.size() - 1);

            // 写入文件
            slot_data_write_into_file(data.latestindex);
        }
    }
    // 更新数据
    else
    {
        _datalist.replace(index, var.value<ScanData>());
    }
}
// 数据写入文件
void DataWorker::slot_data_write_into_file(int index) {
    QString filepath = QString("%1/%2.csv").arg(_dir).arg(index + 1);
    qDebug("slot_data_save: %s", qPrintable(filepath));

    // 初始化文件并打开写入
    QFile *file = new QFile(filepath);
    file->open(QIODevice::ReadWrite | QIODevice::Text);

    // 打开并写入文件
    // 保留有位置信息的的第一行
    qDebug("datalength: %d, valid index: %d", _datalist.size(), _valid_data_indexes[index]);
    ScanData data = _datalist[_valid_data_indexes[index]];

    QString tsstr_spec = QDateTime::fromMSecsSinceEpoch(data.ts_spectrum).toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString tsstr_loc = QDateTime::fromMSecsSinceEpoch(data.ts_location).toString("yyyy-MM-dd HH:mm:ss.zzz");
    // 更新时间
    qint64 diff_ts = data.ts_spectrum - data.ts_location;

    QTextStream out(file);
    out << QString("%1,%2,(%3  %4),location time: %5,spectrum time: %6,time difference: %7")
           .arg(_wavelength[0]).arg(data.spectrum[0]).arg(data.x)
            .arg(tsstr_spec).arg(tsstr_loc).arg(tsstr_spec).arg(diff_ts) << endl;
    for (int i = 1; i < data.spectrum.size(); ++i) {
        out << QString("%1,%2").arg(_wavelength[i]).arg(data.spectrum[i]) << endl;
    }
    file->close();
    delete file;
}
//// 接收光谱数据
//void DataWorker::slot_data_spectrum_update(int *ylist, qint64 ts, int index) {

//}
//// 接收位置数据
//void DataWorker::slot_data_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index) {

//}


ModeMainWindow::ModeMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ModeMainWindow),
    _scan_current(-1),
    _wavelength_range(0, -1),
    _scan_origin_recorded(false)
{
    ui->setupUi(this);
    _UI_update_timer = new QTimer(this);
    connect(_UI_update_timer, SIGNAL(timeout()), this, SLOT(slot_UI_update()));
    _UI_update_timer->start(100);

    // 数据处理线程
    _datasave = new DataWorker;
    _datasave->moveToThread(&_data_thread);
    connect(&_data_thread, &QThread::finished, _datasave, &QObject::deleteLater);
    connect(this, &ModeMainWindow::signal_data_init, _datasave, &DataWorker::slot_data_init);
    connect(this, &ModeMainWindow::signal_data_update, _datasave, &DataWorker::slot_data_update);
    connect(this, &ModeMainWindow::signal_data_write_into_file, _datasave, &DataWorker::slot_data_write_into_file);
    // 在线程之间通讯 注册新类型
    qRegisterMetaType<QList<float>>("QList<float>");

//    connect(this, &ModeMainWindow::signal_data_save, _datasave, &DataWorker::slot_data_save);
    _data_thread.start();

    // 构造平移台控制器
    _modbus = new ModBus();
    this->serial_fetch();

    // 接收串口接收数据信号
    connect(_modbus, SIGNAL(signal_reset_complete(int,int)), this, SLOT(slot_reset_complete(int)));
    connect(_modbus, SIGNAL(signal_location_update(int,int,bool,bool,qint64,int)), this,
            SLOT(slot_location_update(int,int,bool,bool,qint64,int)));
    connect(_modbus, SIGNAL(signal_status_change(int)), this, SLOT(slot_motor_status_change(int)));
    connect(_modbus, SIGNAL(signal_working_state(bool,bool)), this, SLOT(slot_motor_working_state(bool,bool)));
    connect(_modbus, SIGNAL(signal_direction(bool,bool)), this, SLOT(slot_motor_direction(bool,bool)));

    // 光谱图表
    _chart = new Chart();
    _chart->setTitle("spectrum");
    _chart->setAnimationOptions(QChart::AllAnimations);
    _chartview = new QChartView(_chart);
    _chartview->setRenderHint(QPainter::Antialiasing);
    ui->spectrumLayout->addWidget(_chartview);
//    _wavelength_range.first =

    // 奥谱奥谱 一问三不知公司摆烂真离谱
    _op = new OptoTrash();
    connect(_op->opworker, &OptoWorker::signal_spectrum_update, this, &ModeMainWindow::slot_update_spectrum);
    memset(_spectrum_latest, 0, sizeof(_spectrum_latest));

    // 初始化限长
    _x_scan_limit = um2pulse(ui->xLimitSpin->value());
    _y_scan_limit = um2pulse(ui->yLimitSpin->value());

    // 光斑显示窗口
//    _beam_plotw = new PlotWidget;
    // 热力图
    // 设置可以拖拽 / 缩放
    _colormap_latestindex = -1;
    ui->customPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom);
    ui->customPlot->axisRect()->setupFullAxesBox(true);
    ui->customPlot->xAxis->setLabel("x");
    ui->customPlot->yAxis->setLabel("y");
    // 初始化热力图
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

ModeMainWindow::~ModeMainWindow()
{
    delete ui;

    // 断开位移器平台连接
    _modbus->disconnect_();
    delete _modbus;

    // 释放光谱仪
    _op->close();
    // delete _op 会报错，不知道哪里内存泄露了以后再找吧
//    delete _op;

    // 释放数据处理线程
    _data_thread.quit();
    _data_thread.wait();

    // 释放光斑显示窗口
//    delete _beam_plotw;
}

// model函数 串口状态变化
void ModeMainWindow::model_motor_status(ModBus::ModBusStatus motor_status) {
//    qDebug("model update: serial status: %d", motor_status);
    QString color, buttonText, statusText, port;
    port = _modbus->get_port();
    statusText = "位移平台：";
    if (motor_status == ModBus::OK) {
        statusText = statusText.append(port).append("连接成功");
        color = "color: rgb(0, 170, 0)";
        buttonText = "断开平移台";
    } else if (motor_status == ModBus::NOT_CONNECTED) {
        statusText = statusText.append("未连接");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
    } else if (motor_status == ModBus::FAILED) {
        statusText = statusText.append(port).append("连接失败");
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
        qDebug("serial conn failed: ", qPrintable(port));

    } else {
        statusText = statusText.append(port).append(QString("状态错误%d").arg(motor_status));
        color = "color: rgb(170, 0, 0)";
        buttonText = "连接平移台";
        qDebug("serial status error：%d", motor_status);
    }
    ui->serialStatusLabel->setText(statusText);
    ui->serialStatusLabel->setStyleSheet(color);
    ui->serialConnBtn->setText(buttonText);
}

// model 更新坐标显示
void ModeMainWindow::model_xloc_update(int loc) {
    _xloc = loc;
    ui->xLocLabel->setText(QString("(%1,").arg(loc));
}
void ModeMainWindow::model_yloc_update(int loc) {
    _yloc = loc;
    ui->yLocLabel->setText(QString("%1)").arg(loc));
}

// 更新UI
void ModeMainWindow::slot_UI_update()
{
    // 更新光谱折线图
    _chart->update_ylist(_spectrum_latest);

    // 更新位置
    model_xloc_update(pulse2um(_xloc));
    model_yloc_update(pulse2um(_yloc));

    // 运动方向
    if (_xdirection)
        ui->xdirectionLabel->setText("反向");
    else
        ui->xdirectionLabel->setText("正向");
    if (_ydirection)
        ui->ydirectionLabel->setText("反向");
    else
        ui->ydirectionLabel->setText("正向");

    // 运动状态
    if (_xworking)
        ui->xworkingLabel->setText("运行中");
    else
        ui->xworkingLabel->setText("空闲");
    if (_yworking)
        ui->yworkingLabel->setText("运行中");
    else
        ui->yworkingLabel->setText("空闲");
}

// 更新光谱数据
// 并非更新图表，只是写入数据
// 图表更新比较耗时，_chart 会用隔一段时间来自动更新最新数据
void ModeMainWindow::slot_update_spectrum(int *ylist, qint64 ts, int index) {
    // 若处于扫描状态
    // 这里判断_scan_current有所不同
    // 比如_scan_current = 1，则表示已经达到点0，正在往点1前进
    // _scan_current = 0，表示正在往点0前进
    if (_scan_current > -1)
    {
        // 记录光谱数据
        ScanData data;
        data.intensity = 0;
        for (int i = 0; i < _wavelength.size(); ++i) {
            data.spectrum.append(ylist[i]);

            // 根据设定的范围进行累加计算强度值
            if (i > _wavelength_range.first && (_wavelength_range.second == -1 || i < _wavelength_range.second))
                data.intensity += qint64(ylist[i]);
        }
        data.ts_spectrum = ts;

        // 获取当前位置
        data.x = _xloc;
        data.y = _yloc;
        // 扫描位置index
        data.latestindex = _scan_current - 1;

        // 发送信号到子线程保存信息
        QVariant var;
        var.setValue(data);
        emit signal_data_update(var);

        // 在到达第一个点之后，开始更新热力图
        if (_scan_current > 0)
        {
            int xlast = _scan_position[_scan_current - 1][0];
            int ylast = _scan_position[_scan_current - 1][1];

            qDebug("spectrum update, latest arrive: %d, (%d %d)", _scan_current - 1, xlast, ylast);

            // 最新位置的数据采集完成，更新 scan_current - 1 对应的热力图
            // 用_colormap_latestindex控制只更新一次
            if (_colormap_latestindex < _scan_current - 1)
            {
                // 更新图表
                // 更新热力图
                int x = _scan_position[_scan_current - 1][0];
                int y = _scan_position[_scan_current - 1][1];
                qDebug("update color map: (%d, %d), intensity: %d", x, y, data.intensity);
                _colorMap->data()->setCell(x, y, data.intensity);
                _colorMap->rescaleDataRange();
                ui->customPlot->rescaleAxes();
                // rpQueuedReplot避免重复绘制
                ui->customPlot->replot(QCustomPlot::rpQueuedReplot);
                _colormap_latestindex ++;
            }

            // 全部数据扫描完毕，回中点并结束扫描
            if (_scan_current == _scan_location.size())
            {
                // 降低位置信息获取频率
                emit _modbus->signal_auto_location(true, 100);
                // 降低光谱信息获取频率
                emit _op->start_capture_continuous(ui->integralTimeSpin->value(), 100);

                // 移动回中点
                _modbus->movex(_x_scan_origin);
                _modbus->movey(_y_scan_origin);

                qDebug("---------- scan finish ----------");
                qDebug("locations: %d", _scan_location.size());
                qDebug("---------- scan finish ----------");

                // 扫描，关闭！
                _scan_current = -1;
            }
        }
    }
}

// 接收到位移平台状态改变
void ModeMainWindow::slot_motor_status_change(int state) {
//    qDebug("window receive motor status: %d", state);
    model_motor_status(ModBus::ModBusStatus(state));
}

// 接收信号 重置完成
void ModeMainWindow::slot_reset_complete(int motor) {
    QString mo = "X轴";
    if (motor == 1)
        mo = "Y轴";
    ui->statusbar->showMessage(mo.append("归零完成"), 3000);
}
// 接收信号 位置更新
void ModeMainWindow::slot_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index) {
    // 扫描中
    if (_scan_current > -1 && _scan_current < _scan_location.size()) {
        // 读取位置
        qint32 xdest = _scan_location.at(_scan_current).at(0);
        qint32 ydest = _scan_location.at(_scan_current).at(1);
        bool xback = _scan_location.at(_scan_current).at(2);
        bool yback = _scan_location.at(_scan_current).at(3);
        // 判断是否达到指定位置
        bool xarrive = xback ? x <= xdest : x >= xdest;
        bool yarrive = yback ? y <= ydest : y >= ydest;
        // 初始点必须严格到达
        if (_scan_current == 0)
        {
            xarrive = x == xdest;
            yarrive = y == ydest;
        }

        // 扫描点运行状态
        QString xdirection_str = "++++";
        QString ydirection_str = "++++";
        if (xback)
            xdirection_str = "----";
        if (yback)
            ydirection_str = "----";
        qDebug("scan: %d, x: %d/%d %s, y: %d/%d %s", _scan_current, x, xdest,
               qPrintable(xdirection_str), y, ydest, qPrintable(ydirection_str));

        // 到达指定位置
        if (xarrive && yarrive)
        {
            // 记录位置
            _scan_ts_location.append(ts);
            _scan_location[_scan_current].append(x);
            _scan_location[_scan_current].append(y);

            // 处理下一个位置
            // 逐点扫描
            if (ui->scanTypeCheck->currentIndex() != 0)
            {
                // 同步采集光谱
                bool succ = _op->capture_sync(ui->integralTimeSpin->value(), _spectrum_latest, _wavelength.size());
                if (!succ)
                {
                    qCritical("falied to capture spectrum sync");
                    return;
                }

                qDebug("scan index: %d, capture sync success", _scan_current);
                // 记录光谱数据
                ScanData data;
                data.intensity = 0;
                for (int i = 0; i < _wavelength.size(); ++i) {
                    data.spectrum.append(_spectrum_latest[i]);

                    // 根据设定的范围进行累加计算强度值
                    if (i > _wavelength_range.first && (_wavelength_range.second == -1 || i < _wavelength_range.second))
                        data.intensity += qint64(_spectrum_latest[i]);
                }
                data.ts_spectrum = ts;

                // 获取当前位置
                data.x = xdest;
                data.y = ydest;
                data.latestindex = _scan_current;

                // 发送信号到子线程保存信息
                QVariant var;
                var.setValue(data);
                emit signal_data_update(var);

                // 更新光谱折线图
                _chart->update_ylist(_spectrum_latest);

                // 更新热力图
                _colorMap->data()->setCell(
                            _scan_position[_scan_current][0], _scan_position[_scan_current][1], data.intensity);
                _colorMap->rescaleDataRange();
                ui->customPlot->rescaleAxes();
                ui->customPlot->replot(QCustomPlot::rpQueuedReplot);

                // 若已经扫描完
                if (_scan_current == _scan_location.size() - 1)
                {
                    // 降低位置信息获取频率
                    emit _modbus->signal_auto_location(true, 100);
                    // 降低光谱信息获取频率
                    emit _op->start_capture_continuous(ui->integralTimeSpin->value(), 100);

                    // 移动回中点
                    _modbus->movex(_x_scan_origin);
                    _modbus->movey(_y_scan_origin);

                    qDebug("---------- scan finish ----------");
                    qDebug("locations: %d", _scan_location.size());
                    qDebug("---------- scan finish ----------");

                    // 扫描，关闭！
                    _scan_current = -1;
                    return;
                }

                // 更新到下一个点的位置
                qint32 xnext = _scan_location.at(_scan_current + 1).at(0);
                qint32 ynext = _scan_location.at(_scan_current + 1).at(1);

                // 移动到下一个点
                _modbus->movex(xnext);
                _modbus->movey(ynext);
            }

            // 连续扫描
            else
            {
                // 若扫描位置已经走完，等待光谱信息再结束扫描
                if (_scan_current == _scan_location.size() - 1)
                {
                    // 将_scan_current置为过大，不再进行位置处理
                    _scan_current = _scan_location.size();
                    return;
                }

                // 连续扫描
                // 只扫描y轴
                if (ui->xScanTimes->value() == 1)
                {
                    if (_scan_current % ui->yScanTimes->value() == 0)
                    {
                        // y进行下一步
                        qint32 ynext = _scan_location.at(_scan_current + ui->yScanTimes->value() - 1).at(1);
                        qDebug("y move to %d", ynext);
                        _modbus->movey(ynext);
                    }
                }

                // 只扫描x轴
                else if (ui->yScanTimes->value() == 1)
                {
                    if (_scan_current % ui->xScanTimes->value() == 0)
                    {
                        // y进行下一步
                        qint32 xnext = _scan_location.at(_scan_current + ui->xScanTimes->value() - 1).at(0);
                        qDebug("x move to %d", xnext);
                        _modbus->movex(xnext);
                    }
                }

                // 线扫描
                else
                {
                    // 只在y完成一轮时才进行移动
                    if ((_scan_current + 1) % ui->xScanTimes->value() == 0)
                    {
                        // y完成一轮，x进行下一步
                        _modbus->movex(_scan_location.at(_scan_current + 1).at(0));
                    }
                    else if (_scan_current % ui->xScanTimes->value() == 0)
                    {
                        // x完成一次，y进行下一步
                        qint32 ynext = _scan_location.at(_scan_current + ui->yScanTimes->value() - 1).at(1);
                        qDebug("y move to %d", ynext);
                        _modbus->movey(ynext);
                    }
                }
            }

            // 更新到下一步
            _scan_current ++;
        }
        else
        {
            // 未到达
            return;
        }
    }
}

// 电机运动状态
void ModeMainWindow::slot_motor_working_state(bool x, bool y) {
    if (_xworking != x)
        _xworking = x;
    if (_yworking != y)
        _yworking = y;
}
// 电机运动方向
void ModeMainWindow::slot_motor_direction(bool x, bool y) {
    if (_xdirection != x)
        _xdirection = x;
    if (_ydirection != y)
        _ydirection = y;
}

void ModeMainWindow::slot_lead_update(int lead) {
    ui->statusbar->showMessage(QString("设置更新：导程：%1").arg(lead), 3000);
}
void ModeMainWindow::slot_sub_driver_update(int sub) {
    ui->statusbar->showMessage(QString("设置更新：细分：%1").arg(sub), 3000);
}
void ModeMainWindow::slot_step_angle_update(int step) {
    ui->statusbar->showMessage(QString("设置更新：步进角：%1").arg(step), 3000);
}

// 将页面展示的mm转化位um传给电机
int ModeMainWindow::um2pulse(int um) {
    // 螺距 5mm
    // 默认脉冲数 6400
//    return um * 6400 / 5000;
    return um;
}

// 将电机返回的um转化位mm进行显示
int ModeMainWindow::pulse2um(int pulse) {
//    return pulse * 5000 / 6400;
    return pulse;
}
// 获取串口列表
void ModeMainWindow::serial_fetch() {
    // 清除 portBox 已存在的选项
    int existed = ui->serialportBox->count();
    for (int i = 0; i < existed; i++)
        ui->serialportBox->removeItem(0);

    // 添加 portname 到 portBox 选项
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        ui->serialportBox->addItem(info.portName());
    return;
}

// 寻找光斑中心
bool ModeMainWindow::find_beam_center() {
    // 先置零

    // 移动x轴，找到强度最大的点

    // 移动到x轴强度最大点

    // 移动y轴，找到强度最大点

    // 移动到y轴强度最大点

    // 以该点为中心构建扫描
    return false;
}

// 按钮动作 刷新串口列表
void ModeMainWindow::on_serialFetchBtn_clicked() { this->serial_fetch(); }

// 按钮动作 连接 / 断开 串口
void ModeMainWindow::on_serialConnBtn_clicked()
{
    if (_modbus->get_status() == ModBus::OK) {
        // 断开串口连接
        _modbus->disconnect_();
        model_motor_status(ModBus::NOT_CONNECTED);

    } else {
        // 当前状态未连接
        // 获取串口名
        QString portText = ui->serialportBox->currentText();
        // 未选中，直接返回
        if (portText.isNull() || portText.isEmpty()) { return; }
        // 连接串口
        _modbus->connect_(portText);
    }
}

// 按钮动作 向上移动
void ModeMainWindow::on_upBtn_clicked()
{
    int step = um2pulse(ui->moveStepSpin->value());
    _modbus->movey_relative(step);
}

// 按钮动作 向下移动
void ModeMainWindow::on_downBtn_clicked()
{
    int step = um2pulse(ui->moveStepSpin->value());
    _modbus->movey_relative(-step);
}

// 按钮动作 向左移动
void ModeMainWindow::on_leftBtn_clicked()
{
    int step = um2pulse(ui->moveStepSpin->value());
    _modbus->movex_relative(-step);
}

// 按钮动作 向右移动
void ModeMainWindow::on_rightBtn_clicked()
{
    int step = um2pulse(ui->moveStepSpin->value());
    _modbus->movex_relative(step);
}

// 按钮动作 绝对位置移动
void ModeMainWindow::on_movAbsBtn_clicked()
{
    _modbus->movex(um2pulse(ui->xSpin->value()));
    _modbus->movey(um2pulse(ui->ySpin->value()));
}

// 按钮动作 单位改变
// checkBox重复选择当前并不会触发此函数
void ModeMainWindow::on_stepUnitCheck_currentIndexChanged(int index)
{
    if (index == 0) {
        // 从um改成mm
        this->model_xloc_update(_xloc / 1000);
        this->model_yloc_update(_yloc / 1000);
        ui->xSpin->setValue(ui->xSpin->value() / 1000);
        ui->ySpin->setValue(ui->ySpin->value() / 1000);
        ui->moveStepSpin->setValue(ui->moveStepSpin->value() / 1000);
    } else {
        this->model_xloc_update(_xloc * 1000);
        this->model_yloc_update(_yloc * 1000);
        ui->xSpin->setValue(ui->xSpin->value() * 1000);
        ui->ySpin->setValue(ui->ySpin->value() * 1000);
        ui->moveStepSpin->setValue(ui->moveStepSpin->value() * 1000);
    }
    qDebug("unit change: %d", index);
}

// 按钮动作 重置
// 停止电机所有运动，当前位置置为0点
void ModeMainWindow::on_resetBtn_clicked()
{
    _modbus->resetx();
    _modbus->resety();
}

// 按钮动作 启动线扫描
void ModeMainWindow::on_linescanBtn_clicked()
{
    // 记录扫描次数
    int n_xscan = ui->xScanTimes->value();
    int n_yscan = ui->yScanTimes->value();
    if (n_xscan == 1 && n_yscan == 1)
        return;

    // 打开一个目录选择对话框
    // 用来保存该次扫描的数据
    QFileDialog* openFilePath = new QFileDialog( this, "选择保存", "");
    openFilePath-> setFileMode( QFileDialog::Directory );
    QString path;
    if ( openFilePath->exec() == QDialog::Accepted ) {
        QStringList paths = openFilePath->selectedFiles();
        path = paths[0];
    } else {
        return;
    }
    QString date = QDateTime::currentDateTime().toString("MMdd_HH_mm");
    _dirpath = path.append("/").append(date);
    QDir().mkdir(_dirpath);
    qDebug("scan data dir: %s", qPrintable(_dirpath));
    delete openFilePath;

//    _beam_plotw->init(n_xscan, n_yscan);
//    _scan_spectrum_sum.clear();
    // 重置热力图
    _colormap_latestindex = -1;
    int xn = ui->xScanTimes->value();
    int yn = ui->yScanTimes->value();
    _colorMap->data()->clear();
    _colorMap->data()->setSize(xn, yn);
    _colorMap->data()->setRange(QCPRange(0, xn), QCPRange(0, yn));
    ui->customPlot->rescaleAxes();

    // 记录当前点为扫描中心
    _x_scan_origin = _xloc;
    _y_scan_origin = _yloc;
    _scan_origin_recorded = true;
    // 记录边缘点
    int x_start;
    int y_start;
    // 记录当前限长
    _y_scan_limit = ui->yLimitSpin->value();
    _x_scan_limit = ui->xLimitSpin->value();

    // 计算位置点
    _scan_location.clear();
    _scan_position.clear();
    if (n_xscan == 1) {
        // 只扫描y轴
        // 初始点
        x_start = _x_scan_origin;
        y_start = _y_scan_origin - _y_scan_limit / 2;
        // 写入位置
        for (int i = 0; i < n_yscan; ++i) {
            _scan_location.append(
                        {x_start, y_start + i * _y_scan_limit / (n_yscan - 1), false, false});
            _scan_position.append({0, i});
        }

    } else if (n_yscan == 1) {
        // 只扫描x轴
        // 初始点
        x_start = _x_scan_origin - _x_scan_limit / 2;
        y_start = _y_scan_origin;
        // 写入位置
        for (int i = 0; i < n_xscan; ++i) {
            _scan_location.append(
                        {x_start + i * _x_scan_limit / (n_xscan - 1), y_start, false, false});
            _scan_position.append({i, 0});
        }

    } else {
        // 双轴扫描
        // 初始点
        x_start = _x_scan_origin - _x_scan_limit / 2;
        y_start = _y_scan_origin - _y_scan_limit / 2;
        // 写入位置
        for (int i = 0; i < n_xscan; ++i) {
            // x轴位置
            int xdest = x_start + i * _x_scan_limit / (n_xscan - 1);
            for (int j = 0; j < n_yscan; ++j) {
                // y轴位置
                int ydest = y_start + j * _y_scan_limit / (n_yscan - 1);
                int ypos = j;
                bool yback = false;
                // y轴反向
                if (i % 2 != 0) {
                    ydest = y_start + (n_yscan - 1 - j) * _y_scan_limit / (n_yscan - 1);
                    ypos = n_yscan - 1 - j;
                    yback = true;
                }
                _scan_location.append({xdest, ydest, false, yback});
                _scan_position.append({i, ypos});
            }
        }
    }
    int nscan = _scan_location.size();
    // 重复扫描
    for (int var = 1; var < ui->scanLoopSpin->value(); ++var) {
        int last = _scan_location.size() - 1;
        for (int j = 0; j < nscan; ++j) {
            QList<qint32> copy = _scan_location.at(last - j);
            copy[2] = !copy[2];
            copy[3] = !copy[3];
            _scan_location.append(copy);
            _scan_position.append(_scan_position.at(last - j));
        }
    }

    // 打印扫描路径
    for (int i = 0; i < _scan_location.size(); ++i) {
        if (_scan_location[i][3])
            qDebug("index: %d, x: %d, y: %d, ----", i + 1, _scan_position[i][0], _scan_position[i][1]);
        else
            qDebug("index: %d, x: %d, y: %d, ++++", i + 1, _scan_position[i][0], _scan_position[i][1]);
    }

    // 初始化数据处理
    emit signal_data_init(_wavelength, _wavelength_range.first, _wavelength_range.second, _dirpath);

    // 修改速度
    _xvelocity = um2pulse(ui->xSpeedSpin->value());
    _yvelocity = um2pulse(ui->ySpeedSpin->value());
    _modbus->velocity(_xvelocity, 0);
    _modbus->velocity(_yvelocity, 1);

    // 移动到边缘
    if (n_xscan > 1)
        _modbus->movex(_x_scan_origin - _x_scan_limit / 2);
    if (n_yscan > 1)
        _modbus->movey(_y_scan_origin - _y_scan_limit / 2);

    // 加快位置信息获取频率
    emit _modbus->signal_auto_location(true, ui->scanIntervalSpin->value());
    // 加快光谱信息获取频率
    // 若是连续采集，加快光谱获取
    if (ui->scanTypeCheck->currentIndex() == 0)
        emit _op->start_capture_continuous(ui->integralTimeSpin->value(), 0);
    // 若是定点扫描，用同步方式获取光谱，并主动更新图表
    else
        _op->stop_capture_continuous();

    // 扫描，启动！
    _scan_current = 0;
//    _n_xcurrent = 0;
//    _n_ycurrent = 0;
}

// 按钮动作 打开光谱仪
void ModeMainWindow::on_btnOpenDev_clicked()
{
    if(_op->is_device_open()) {
        _op->close();
        ui->btnOpenDev->setText("打开设备");
    } else {
        bool succ = _op->open();
        qDebug("device open: %d", succ);
        if (succ) {
            ui->btnOpenDev->setText("关闭设备");
            // 获取波长并初始化图表
            _wavelength.clear();
            for (int i = 0; i < _op->get_resolution(); i++)
                _wavelength.append(_op->get_wavelength()[i]);
            _chart->init(_op->get_wavelength(), _op->get_resolution());
        }
        else
            _op->close();
    }
}

// 按钮动作 单次采集
void ModeMainWindow::on_btnSingleShot_clicked() {
    _op->capture_once(ui->integralTimeSpin->value());
}

// 按钮动作 采集暗底
void ModeMainWindow::on_btnDarkShot_clicked() {
    _op->capture_dark(ui->integralTimeSpin->value());
}

// 按钮动作 连续采集
void ModeMainWindow::on_btnContinues_clicked() {
    _op->start_capture_continuous(ui->integralTimeSpin->value(), 100);
}

// 按钮动作 停止连续采集
void ModeMainWindow::on_btnStopShot_clicked() {
    _op->stop_capture_continuous();
}

// 按钮动作 清除图像
void ModeMainWindow::on_btnClear_clicked()
{
//    QList<QPointF> pointlist = {};
//    m_chart->updateSeries(pointlist);
}

// 按钮动作 停止运动
void ModeMainWindow::on_stopBtn_clicked()
{
    _modbus->stopx();
    _modbus->stopy();
}

// 按钮动作 记录原点
void ModeMainWindow::on_manualOriginBtn_clicked()
{
    // 设置最小坐标
    _xmin = _xloc;
    _ymin = _yloc;
    // 设置最大坐标
    _xmax = _xmin + _x_scan_limit;
    _ymax = _ymin + _y_scan_limit;
    // 记录偏移，提供给界面显示
//    _xoffset = _xloc;
//    _yoffset = _yloc;
//    qDebug("offset: %d, %d", _xoffset, _yoffset);
}

// 按钮动作 修改速度
void ModeMainWindow::on_setSpeedBtn_clicked()
{
    _xvelocity = um2pulse(ui->xSpeedSpin->value());
    _yvelocity = um2pulse(ui->ySpeedSpin->value());
    _modbus->velocity(_xvelocity, 0);
    _modbus->velocity(_yvelocity, 1);
}

// 按钮动作 修改限长
//void ModeMainWindow::on_setLimitBtn_clicked()
//{
//    _y_scan_limit = ui->yLimitSpin->value();
//    _x_scan_limit = ui->xLimitSpin->value();
//}

// 按钮动作 修改图表坐标范围
void ModeMainWindow::on_setChartcoordinateBtn_clicked()
{
    _chart->set_x_range(ui->xMinSpin->value(), ui->xMaxSpin->value());
    _chart->set_y_range(ui->yMinSpin->value(), ui->yMaxSpin->value());

    // 更新光谱范围
    bool start = false;
    bool end = false;
    int index = 0;
    while (!start || !end)
    {
        // 超出范围
        if (index >=_wavelength.size())
            return;

        // 正序找到第一个比设定值大的位置
        if (!start && _wavelength[index] > ui->xMinSpin->value())
        {
            qDebug("find min x %f>%d at %d", _wavelength[index], ui->xMinSpin->value(), index);
            _wavelength_range.first = index;
            start = true;
        }
        // 倒序找到第一个比设定值小的位置
        if (!end && _wavelength[_wavelength.size() - 1 - index] < ui->xMaxSpin->value())
        {
            qDebug("find max x %f<%d at %d",
                   _wavelength[_wavelength.size() - 1 - index], ui->xMaxSpin->value(), _wavelength.size() - 1 - index);
            _wavelength_range.second = _wavelength.size() - 1 - index;
            end = true;
        }
        index ++;
    }
}

// 按钮动作 记录中心
void ModeMainWindow::on_recordScanOriginBtn_clicked()
{
    _x_scan_origin = _xloc;
    _y_scan_origin = _yloc;
    _scan_origin_recorded = true;
}

// 按钮动作 回到中心
void ModeMainWindow::on_toScanOriginBtn_clicked()
{
    if (_scan_origin_recorded) {
        _modbus->movex(_x_scan_origin);
        _modbus->movey(_y_scan_origin);
    }
}

// 按钮动作 显示光斑
void ModeMainWindow::on_scanBeamPlotDlg_clicked()
{
//    if (_beam_plotw->isHidden())
//        _beam_plotw->show();
//    else
//        _beam_plotw->hide();
}

