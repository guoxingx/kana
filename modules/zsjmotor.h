#ifndef ZSJMOTOR_H
#define ZSJMOTOR_H

#include <QSerialPort>
#include <QThread>
//#include <QString>

// 电机间隔，ms，轮询 / 更新 的最小间隔
//static int INTERVAL = 100;
// 超时时间，ms，单次操作的超时时间
//static int TIMEOUT = 30000;


// 异步处理线程
class MotorCommunication: public QObject
{
    Q_OBJECT
public:
    explicit MotorCommunication(QObject *parent = nullptr);
    ~MotorCommunication();

signals:
    void signal_comm_received(QByteArray buf);
    void signal_comm_status_change(int state);
//    void signal_comm_send_result(int err);

protected slots:
    void init();
    void slot_connect(QString port);
    void slot_disconnect();
    void slot_send(QByteArray buf);
    void slot_handle_serial_received();

private:
    QSerialPort *_serial;
};

class ZSJMotor: public QObject
{
    Q_OBJECT

public:
    explicit ZSJMotor(QObject *parent = nullptr);
    ~ZSJMotor();

    // 连接状态
    enum MotorStatus {
        STATUS_FAILED = -1,
        STATUS_NOT_CONNECTED = 0,
        STATUS_OK = 1,
    };

    // 错误码
    enum MotorErr {
        ERR_NOT_IMPLEMENTED = -1,
        ERR_OK = 0,
        ERR_TIMEOUT = 1,
        ERR_NOT_CONNECTED = 2,
        ERR_PARAMS_ERROR = 3,
    };

    // X Y 轴操作
    enum XYACTION {
        XONLY = 0,
        YONLY = 1,
        XYBOTH = 2,
    };

    // 连接
    void connect_(QString port);
    // 断开连接
    void disconnect_();
    // 获取状态
    MotorStatus get_status() { return _status; }
    // 获取当前连接的串口名
    QString get_port() { return _serialport; }
    // 获取当前坐标
    int get_xlocation() { return _xloc; }
    int get_ylocation() { return _yloc; }
    // 获取目的地
    int get_xdestination() { return _xdest; }
    int get_ydestination() { return _ydest; }
    // 电机工作状态
    bool is_xmotor_working() { return _xworking; }
    bool is_ymotor_working() { return _yworking; }
    // 获取限位坐标
    bool get_xmax() { return _xmax; }
    bool get_xmin() { return _xmin; }
    bool get_ymax() { return _ymax; }
    bool get_ymin() { return _ymin; }

    // 归零
    void reset(XYACTION action=XONLY);
    void resetx() { return reset(XONLY); }
    void resety() { return reset(YONLY); }
    // 当前位置
    void location(XYACTION action=XONLY);
    void locationx() { return location(XONLY); }
    void locationy() { return location(YONLY); }
    // 移动
    void move(int dest, XYACTION action=XONLY);
    void move(int x, int y);
    void movex(int dest) { return move(dest, XONLY); }
    void movey(int dest) { return move(dest, YONLY); }
    // 设置导程
    void set_lead(int lead);
    // 设置细分
    void set_sub_driver(int sub);
    // 设置步进角
    void set_step_angle(int step);

protected slots:
    // 接收到子线程返回
    void slot_handle_comm_receive(QByteArray buf);
    void slot_handle_comm_status(int status);
//    void slot_handle_comm_send_result(int succ);

signals:
    // 对外的回调函数
    void signal_reset_complete(int motor);
    void signal_location_complete(int loc, int motor);
    void signal_move_complete(int loc, int motor);
    void signal_lead_update(int lead);
    void signal_sub_driver_update(int sub);
    void signal_step_angle_update(int step);
    void signal_destination_update(int dest, int motor);
    void signal_limit_max_update(int dest, int motor);
    void signal_limit_min_update(int dest, int motor);
    void signal_motor_status_change(int state);
    // 对子线程的操作
    void signal_connect(QString port);
    void signal_disconnect();
    void signal_write_msg(QByteArray buf);

private:
    // 当前状态，0：未连接；1：连接成功；
    MotorStatus _status;
    // 当前连接的串口名，未连接用空字符串表示
    QString _serialport;
    // status更新函数
    void model_status(MotorStatus status);
    void model_status(MotorStatus status, QString port);

    // 判断设置的坐标是否超过限制
    bool is_xloc_available(int loc);
    bool is_yloc_available(int loc);
    // 如果设置的坐标是否超过限制，返回最大限制；否则返回原值
    int xloc_available(int loc);
    int yloc_available(int loc);

    // 当前坐标
    int _xloc = 0;
    int _yloc = 0;
    // 当前目的地
    int _xdest = 0;
    int _ydest = 0;
    // x y 工作状态
    bool _xworking = false;
    bool _yworking = false;
    // x y 限位坐标
    int _xmin = 0;
    int _xmax = 0;
    int _ymin = 0;
    int _ymax = 0;

    // 串口对象
//    QSerialPort *_serial;
    // 串口读写线程和对象
    QThread _thread;
    MotorCommunication *_comm;

    // 发送指令，同步，返回错误码。超时返回超时错误
    void send(const char *msg, int len=9);
};


#endif // ZSJMOTOR_H
