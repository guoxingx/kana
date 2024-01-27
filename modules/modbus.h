#ifndef MODBUS_H
#define MODBUS_H

#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QTimer>

// 异步处理线程
class ModBusComm: public QObject
{
    Q_OBJECT
public:
    explicit ModBusComm(QObject *parent = nullptr);
    ~ModBusComm();

signals:
    void signal_comm_received(QByteArray buf);
    void signal_comm_status_change(int status);
    void signal_comm_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index);
    void signal_comm_reach_origin(int loc, int channel);
    // 运动状态信号 发送给控制器
    void signal_comm_working_state(bool x, bool y);
    // 方向信号 发送给控制器
    void signal_comm_direction(bool x, bool y);

protected slots:
    void init();
    void slot_connect(QString port);
    void slot_disconnect();
    void slot_send(QByteArray buf);
    void slot_handle_serial_received();
    // 接收 打开 / 关闭 自动更新位置信息
    void slot_auto_location(bool open, int ms);

    // 定时器获取位置
    void slot_request_location();
    // 主动获取位置
    void slot_acquire_location(int index);

private:
    // 串口对象
    QSerialPort *_serial;
    // 串口读取的数据
    QByteArray _serial_read;
    // 发送池
    // modbus必须是一问一答的方式
    // 因此多条指令必须等待顺序返回再写入
    QList<QByteArray> _sendpool;
    // 计时器获取位置信息
    QTimer *_timer;

    // 主动获取位置标记位
    int _index;

    // 记录运动状态，发生改变的时候emit signal_comm_working_state
    bool _xworking;
    bool _yworking;
    // 记录方向状态，发生改变的时候emit signal_comm_direction
    bool _xdirection;
    bool _ydirection;
};


// 控制器
// channel: 0 / 1 / 2 / 3 即对应的四轴驱动
class ModBus : public QObject
{
    Q_OBJECT
public:
    explicit ModBus(QObject *parent = nullptr);
    ~ModBus();

    enum ModBusStatus {
        FAILED = -1,
        NOT_CONNECTED = 0,
        OK = 1,
    };

    // 驱动指令
    void connect_(QString port);
    void disconnect_();

    void reset(int channel);
    void resetx() { return reset(0); }
    void resety() { return reset(1); }

    void move(int dest, int channel);
    void movex(int dest) { return move(dest, 0); }
    void movey(int dest) { return move(dest, 1); }

    void stop(int channel);
    void stopx() { return stop(0); }
    void stopy() { return stop(1); }

    void move_relative(int dis, int channel);
    void movex_relative(int dis) { return move_relative(dis, 0); }
    void movey_relative(int dis) { return move_relative(dis, 1); }
    // 设置速度
    void velocity(int v, int channel, int acceleration=200);
    // 设置微步细分
    void subdivision(int v, int channel);

    // 获取状态信息等
    ModBusStatus get_status() { return _status; }
    QString get_port() { return _serialport; }
    // 主动获取位置
    void acquire_location(int index) { emit signal_acquire_location(index); }

signals:
    // 对外的接口
    void signal_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index);
    void signal_status_change(int state);
    void signal_destination_update(int dest, int channel);
    void signal_reset_complete(int loc, int channel);
    // 运动状态信号
    void signal_working_state(bool x, bool y);
    // 方向信号
    void signal_direction(bool x, bool y);

    // 对子线程的操作
    void signal_connect(QString port);
    void signal_disconnect();
    void signal_write_msg(QByteArray buf);
    void signal_acquire_location(int index);
    // 自动更新位置信息 开关 & 速率
    void signal_auto_location(bool open, int ms=100);

protected slots:
    // 接收到子线程返回
    void slot_handle_comm_status(int status);
    void slot_handle_comm_location_update(int x, int y, bool x_valid, bool y_valid, qint64 ts, int index) {
        emit signal_location_update(x, y, x_valid, y_valid, ts, index);
    }
    void slot_handle_comm_reach_origin(int loc, int channel) { emit signal_reset_complete(loc, channel); }

    // 运动状态信号 发送给控制器
    void slot_handle_comm_working_state(bool x, bool y) { emit signal_working_state(x, y); }
    // 方向信号 发送给控制器
    void slot_handle_comm_direction(bool x, bool y) { emit signal_direction(x, y); }

private:
    // 子线程
    ModBusComm *_comm;
    QThread _thread;

    // 当前状态，0：未连接；1：连接成功；
    ModBusStatus _status;
    // 当前连接的串口名，未连接用空字符串表示
    QString _serialport;
    // status更新函数
    void model_status(ModBusStatus status);
    void model_status(ModBusStatus status, QString port);

    void write();
};

#endif // MODBUS_H
