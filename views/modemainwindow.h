#ifndef MODEMAINWINDOW_H
#define MODEMAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QValueAxis>
#include <QThread>
#include <QTimer>

#include "modules/modbus.h"
#include "views/chart.h"
#include "views/plotwidget.h"
#include "modules/optotrash.h"


namespace Ui {
class ModeMainWindow;
}

// 单次扫描数据
typedef struct {
    // 光谱数据
    QList<qint32> spectrum;
    // 光强度，可能在选择波段后累加
    qint64 intensity;
    // 位置信息
    qint32 x;
    qint32 y;
    // 上一个扫描点index
    int latestindex;
    // 时间信息
    qint64 ts_spectrum;
    qint64 ts_location;
} ScanData;
Q_DECLARE_METATYPE(ScanData)

//Q_DECLARE_METATYPE(QPair<int, int>)

class DataWorker : public QObject
{
    Q_OBJECT

signals:
    void signal_UI_update_spectrum(QVariant data);

public slots:
    // 初始化波段
    void slot_data_init(QList<float> wavelength, int start, int end, QString dir);
    // 更新数据，index=-1表示添加到末尾，否则替换对应index位置的数据
    void slot_data_update(QVariant var, int index=-1);
    // 数据写入文件
    void slot_data_write_into_file(int index);
    // 接收光谱数据
//    void slot_data_spectrum_update(int *ylist, qint64 ts, int index);
    // 接收位置数据
//    void slot_data_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index);

private:
    // 扫描数据
    QList<ScanData> _datalist;
    // 扫描的光谱
    QList<float> _wavelength;
    // 扫描的光谱范围选择
    QPair<int, int> _wavelength_range;
    // 有效扫描点，记录_datalist中对应的index
    QList<int> _valid_data_indexes;
    // 扫描位置信息 0: x位置点; 1: y位置点; 2: x反向; 3: y反向; 4: x实际位置 5: y实际位置
    QList<QList<qint32>> _locations;
    // 保存目录
    QString _dir;
};

class ModeMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ModeMainWindow(QWidget *parent = nullptr);
    ~ModeMainWindow();

signals:
    // 数据处理线程初始化波段
    void signal_data_init(QList<float> wavelength, int start, int end, QString dir);
    // 数据处理线程更新数据
    void signal_data_update(QVariant var, int index=-1);
    // 数据处理线程写入文件
    void signal_data_write_into_file(int index);
//    void signal_data_save(QString filepath, int x, int y, QList<qint32> *spectrum, qint64 loc_ts, qint64 spec_ts);



private slots:
    // 更新UI，主要是电机和光谱信息
    void slot_UI_update();
    // 槽函数 位移平台信息更新
    void slot_reset_complete(int motor);
    void slot_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index);
    void slot_lead_update(int lead);
    void slot_sub_driver_update(int sub);
    void slot_step_angle_update(int step);
    void slot_motor_status_change(int state);
    // 电机运动状态
    void slot_motor_working_state(bool x, bool y);
    // 电机运动方向
    void slot_motor_direction(bool x, bool y);

    // 光谱信息更新
    void slot_update_spectrum(int *ylist, qint64 ts, int index);

    void on_serialFetchBtn_clicked();

    void on_serialConnBtn_clicked();

    void on_upBtn_clicked();

    void on_downBtn_clicked();

    void on_leftBtn_clicked();

    void on_rightBtn_clicked();

    void on_movAbsBtn_clicked();

    void on_stepUnitCheck_currentIndexChanged(int index);

    void on_resetBtn_clicked();

    void on_linescanBtn_clicked();

    void on_btnOpenDev_clicked();

    void on_btnSingleShot_clicked();

    void on_btnContinues_clicked();

    void on_btnDarkShot_clicked();

    void on_btnStopShot_clicked();

    void on_btnClear_clicked();

    void on_stopBtn_clicked();

    void on_manualOriginBtn_clicked();

    void on_setSpeedBtn_clicked();

    void on_setChartcoordinateBtn_clicked();

    void on_recordScanOriginBtn_clicked();

    void on_toScanOriginBtn_clicked();

    void on_scanBeamPlotDlg_clicked();

private:
    Ui::ModeMainWindow *ui;

    // 用计时器更新ui
    QTimer *_UI_update_timer;

    // 光谱图表
    Chart *_chart;
    QChartView *_chartview;
    // 光谱组件
    OptoTrash *_op;
    // 最新保持的光谱数据
    int _spectrum_latest[4096];
    // 光谱
    QList<float> _wavelength;
    // 光谱范围，记录的是_wavelength中对应的index
    QPair<int, int> _wavelength_range;
    // 扫描采集时的全量数据
//    QList<ScanData> _scandata_list;
//    QList<QList<qint32>> _scan_data_spectrum;
//    QList<qint64> _scan_ts_spectrum;
//    QList<qint64> _scan_spectrum_sum;

    // 主动获取数据的标志位
//    int _acquire_index;
    // 数据处理线程
    QThread _data_thread;
    // 数据处理worker
    DataWorker *_datasave;
    // 保存数据的文件夹路径
    QString _dirpath;
//    void save_current_data();
//    int *_spectrums[4096];
//    int _nbytes_spectrums;

    // 平移台
    ModBus *_modbus;
    void model_motor_status(ModBus::ModBusStatus status);
    int _xloc = 0;
    void model_xloc_update(int loc);
    int _yloc = 0;
    void model_yloc_update(int loc);
    qint64 _latest_location_ts;
    bool _xworking;
    bool _yworking;
    bool _xdirection;
    bool _ydirection;
    // 扫描采集时的位置数据
//    QList<QPair<int, int>> _scan_data_location;
    QList<qint64> _scan_ts_location;

    // 偏移量
    // 在执行原点操作后，电机会往负方向走，直到触发限位开关
    // 记录该点的脉冲数，记为偏移量
    // 界面上显示会把原点置为(0, 0)
    // 之后指定位移操作都会添加偏移量
//    int _xoffset;
//    int _yoffset;
    // 记录的速度
    int _xvelocity;
    int _yvelocity;

    // 记录可输入的范围
    int _xmin;
    int _xmax;
    int _ymin;
    int _ymax;
    // 扫描类型
//    int _scan_type;
    // 扫描点 0: x位置点; 1: y位置点; 2: x反向; 3: y反向; 4: x实际位置 5: y实际位置
    QList<QList<qint32>> _scan_location;
    // 扫描点 对应序列点，x编号，y编号
    QList<QList<qint32>> _scan_position;
    // 扫描步长
    qint32 _scan_xstep;
    qint32 _scan_ystep;
    // 当前扫描次数
    int _scan_current;
//    int _n_xscan;
//    int _n_yscan;
//    int _n_xcurrent;
//    int _n_ycurrent;
    // 扫描的限长
    int _x_scan_limit;
    int _y_scan_limit;
    // 扫描中心点
    int _x_scan_origin;
    int _y_scan_origin;
    bool _scan_origin_recorded;

    // 单位处理，将 显示单位 - 实际运算值 进行转化
    int um2pulse(int um);
    int pulse2um(int pulse);
    // 刷新串口
    void serial_fetch();
    // 寻找中心
    bool find_beam_center();

//    PlotWidget *_beam_plotw;
    // 热力图
    QCPColorMap *_colorMap;
    QCPColorScale *_colorScale;
    int _colormap_latestindex;
};

#endif // MODEMAINWINDOW_H
