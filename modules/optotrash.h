#ifndef OPTOTRASH_H
#define OPTOTRASH_H

#include <QObject>
#include <QThread>
#include <QTimer>

// 先引用 DriverType 再引用 Driver_app
// 否则会编译错误 c2378重定义
#include "DriverType.h"
#include "Driver_app.h"

// 单次任务执行
class OptoWorker : public QObject {
    Q_OBJECT
public:
    explicit OptoWorker(QObject *parent = nullptr);
    ~OptoWorker();

signals:
    // 对外的信号
    void signal_spectrum_update(int *spectrum, qint64 ts, int index);
    void signal_spectrum_dark_update(int *dark, qint64 ts);
    // 对控制器的信号
    void signal_state_change(int state);
protected slots:
    void slot_capture_once(int integral_ms, int index);
    void slot_capture_once();
    void slot_capture_dark(int integral_ms);
    void slot_capture_continuous_start(int integral_ms, int interval_ms);
    void slot_capture_continuous_stop();

protected:
    // loop中重复调用
    // 返回true表示可以继续执行
    // 返回false表示停止循环
    bool capture(int index=-1);
    bool capture_dark();

private:
    // 用timer来控制连续采样，而不是把子线程的事件循环给阻塞掉
    QTimer *_timer;
    // 积分时间
    int _integral_ms;
    // 设备状态 应该和控制器保持一致
    int _device_state;
    void model_device_state(int s) { _device_state = s; emit signal_state_change(s); }

    // 奥普的接口需要主动查询ready标记位
    // 设置的查询间隔
    int _ready_flag_ms = 1;
};

class OptoTrash : public QObject
{
    Q_OBJECT

public:
    explicit OptoTrash(QObject *parent = nullptr);
    ~OptoTrash();

    // 设备状态
    enum DeviceState {
        Dev_Idle = 0,
        Dev_Running = 1,
        Dev_Initializing = 2,
        Dev_InitializeFail = 3,
        Dev_Opened = 4,
        Dev_Disconnected = 97,
        Dev_PlugIn = 98,
        Dev_PlugOff = 99
    };

    // 打开 / 关闭
    bool open();
    bool close();
    bool is_device_open();
    // 获取精度，5040是12位，4096个点
    int get_resolution() { return m_ccdsize; }
    // 在open之后会更新波长数据
    float *get_wavelength() { return m_wavelength; }
    // 获取最新保存的光谱数据
    int *get_latest_spectrum() { return m_spectrum; }

    // 设置积分时间 单位us
    bool set_integral_time_milli(int ms);
    // 单次采集
    bool capture_once(int integral_ms, int index=-1);
    bool capture_sync(int integral_ms, int *spectrum, int length);
    // 获取暗光谱
    bool capture_dark(int integral_ms);
    // 启动连续采集
    bool start_capture_continuous(int integral_ms, int interval_ms);
    // 关闭连续采集
    bool stop_capture_continuous();

    OptoWorker *opworker;

signals:
    // 结合 worker 对外信号

    // 对子线程的信号
    // 使用 signals 标记信号函数，信号是一个函数声明，返回 void，不需要实现函数代码
    void signal_capture_once(int integral_ms, int index);
    void signal_capture_dark(int integral_ms);
    void signal_capture_continuous_start(int integral_ms, int interval_ms);
    void signal_capture_continuous_stop();
    // 更新设备状态
    void signal_device_state_update(int state);

protected slots:
    // 槽函数是普通的成员函数，作为成员函数，会受到 public、private、protected 的影响
    // protected 只能在当前包的所有类以及子类中被调用
    // 这里的槽是用来接收worker的返回，不被外界调用
//    void slot_handle_worker_result(int *spectrum, qint64 ts, int index);
//    void slot_handle_worker_result_dark(int *dark, qint64 ts);
    void slot_handle_device_state_change(int state) { _device_state = DeviceState(state); }

private:
    // 积分时间 毫秒
    int m_intergral_time_milli;

    // 最近一次采集信息
    float m_wavelength[SPECTRUM_SIZE];
    float m_spectrum_dark[SPECTRUM_SIZE];
    int m_spectrum[SPECTRUM_SIZE];

    // 设备状态
    DeviceState _device_state;
    // ccd尺寸，应该和SPECTRUM_SIZE一致，是一个线阵ccd
    int m_ccdsize;
    // 产品序列号
    QString m_SN;

    // 子线程管理
    QThread m_opthread;
};

#endif // OPTOTRASH_H
