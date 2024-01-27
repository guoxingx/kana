#include "zsjmotor.h"
#include "modules/charutils.h"

// 通信协议，一共9bytes
const char CMD_FLAG[2]  = {0xaa, 0x55};
const char CMD_ACTION   = 0x00;
const char CMD_VELOCITY = 0x00;
const char CMD_DATA[4]  = {0x00, 0x00, 0x00, 0x00};
const char CMD_CHECKSUM = 0x00;

const char CMD_BASE[9] = {0xaa, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const char CMD_ACTION_RESET      = 0x00;
const char CMD_ACTION_LOCATION   = 0x01;
const char CMD_ACTION_MOVE       = 0x02;
const char CMD_ACTION_LEAD       = 0xff;
const char CMD_ACTION_SUBDRIVER  = 0xfe;
const char CMD_ACTION_STEPANGLE  = 0xfd;

const char CMD_XY_GAP = 0x03;

// 返回命令基本格式
void cmd_base(char* cmd, const char action=0x00) {
    cmd[0] = CMD_FLAG[0];
    cmd[1] = CMD_FLAG[1];
    cmd[2] = action;
    cmd[3] = CMD_VELOCITY;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    cmd[8] = CMD_CHECKSUM;
}

//// 校验和
//// len表示作为累加的bytes数，计算完后会在arr[len]直接写入校验和
//char checksum(char arr[], int len=8) {
//    int sum = 0;
//    for(int i = 0; i < len; ++i)
//       sum += int(arr[i]);
//    sum = sum % 256;
//    arr[len] = sum;
////    arr[len] = *((char *)(&sum) + 0);
//    return sum;
//}

//// 将 num 写入 arr 的后续 len 个 bytes
//void int2char(char arr[], int num, int len=4) {
//    for (int i = 0; i < len; ++i)
//        arr[len - 1 - i] = *((char *)(&num) + i);
//}

//// 将 arr 后续的 len 个 bytes 转为 int 并返回
//int char2int32(char arr[], int start=4) {
//    int num = *(int32_t *)(&(arr[start]));

//    // 小端转大端
//    return (num & 0xff) << 24 | (num & 0xff00) << 8 | (num & 0xff0000) >> 8 | (num >> 24) & 0xff;
//}

//// 将 arr 转化为 QString
//QString uchar2str(const unsigned char *arr, int len=9) {
//    QString fuckcpp;
//    for (int i = 0; i < len; ++i) {
//        if (arr[i] < 0x10)
//            fuckcpp += QString("0%1").arg(arr[i], 0, 16);
//        else
//            fuckcpp += QString("%1").arg(arr[i], 0 ,16);
//    }
//    return fuckcpp;
//}

//QString QByteArray2hex(QByteArray buf) {
//    QString hex;
//    for (int i = 0; i < buf.size(); ++i) {
//        if (uchar(buf.at(i)) < 0x10)
//            hex += QString("0%1").arg(uchar(buf.at(i)), 0, 16);
//        else
//            hex += QString("%1").arg(uchar(buf.at(i)), 0 ,16);
//    }
//    return hex;
//}

//// C89标准规定，short和char会被自动提升为int（整形化，类似地，float也会自动提升为double）
//// 这样做是为了便于编译器进行优化，使变量的长度尽可能一样，尽可能提升所产生代码的效率
//// char的值当它是正数的时候也同样进行了符号扩展的，只不过正数是前面加0，用%02x打印的时候那些0被忽略；
//// 而补码表示的负数的符号扩展却是前面加1，用%02x打印的时候那些1不能被忽略，因此才按照本来的长度输出来。
//// 如没有添加unsigned，则当char>0x7F时（如0X80），格式转换为FFFFFF80！
//// 解决办法：在char 前面加上 unsigned.
//QString char2str(const char *arr, int len=9) {
//    QString fuckcpp;
//    for (int i = 0; i < len; ++i) {
//        if (uchar(arr[i]) < 0x10)
//            fuckcpp += QString("0%1").arg(uchar(arr[i]), 0, 16);
//        else
//            fuckcpp += QString("%1").arg(uchar(arr[i]), 0 ,16);
//    }
//    return fuckcpp;
//}

// 将x轴马达转为y轴
void x2y(char arr[], int pos=2) { arr[pos] += CMD_XY_GAP; }

// 构造函数
ZSJMotor::ZSJMotor(QObject *parent) : QObject(parent) {
    // 串口连接初始化
    _status = STATUS_NOT_CONNECTED;
    _serialport.clear();

    // 异步串口
    _comm = new MotorCommunication;
    _comm->moveToThread(&_thread);
    connect(&_thread, SIGNAL(started()), _comm, SLOT(init()));
    connect(&_thread, &QThread::finished, _comm, &QObject::deleteLater);
    connect(this, SIGNAL(signal_write_msg(QByteArray)), _comm, SLOT(slot_send(QByteArray)));
    connect(this, SIGNAL(signal_connect(QString)), _comm, SLOT(slot_connect(QString)));
    connect(this, SIGNAL(signal_disconnect()), _comm, SLOT(slot_disconnect()));
    connect(_comm, SIGNAL(signal_comm_received(QByteArray)), this, SLOT(slot_handle_comm_receive(QByteArray)));
    connect(_comm, SIGNAL(signal_comm_status_change(int)), this, SLOT(slot_handle_comm_status(int)));

    _thread.start();
}

// 析构函数
ZSJMotor::~ZSJMotor() {
    // 释放串口连接
    disconnect_();

    // 释放串口异步
    _thread.quit();
    _thread.wait();
}

// 连接
void ZSJMotor::connect_(QString port) {
    // 串口名
    if (port.isNull() || port.isEmpty()) { return; }
    model_status(STATUS_NOT_CONNECTED, port);
    emit signal_connect(port);
}

// 断开连接
void ZSJMotor::disconnect_() {
    // 关闭串口，但是子线程不退出
    emit signal_disconnect();
    _serialport.clear();

    // 重置状态
    _xdest = 0; _ydest = 0;
    _xloc = 0; _yloc = 0;
    _xworking = false; _yworking = false;

    // 更新model
    model_status(STATUS_NOT_CONNECTED);
}

// status更新函数
void ZSJMotor::model_status(MotorStatus status) {
    if (status == _status) return;
    _status = status;
}
void ZSJMotor::model_status(MotorStatus status, QString port) {
    if (status == _status && port.compare(_serialport) == 0) return;
    _status = status;
    _serialport = port;
}

// 判断设置的坐标是否超过限制
bool ZSJMotor::is_xloc_available(int loc) {
    if (loc > _xmax || loc < _xmin)
        return false;
    return true;
}
bool ZSJMotor::is_yloc_available(int loc) {
    if (loc > _ymax || loc < _ymin)
        return false;
    return true;
}
// 如果设置的坐标是否超过限制，返回最大限制；否则返回原值
int ZSJMotor::xloc_available(int loc) {
    if (loc > _xmax)
        return _xmax;
    else if (loc < _xmin)
        return _xmin;
    return loc;
}
int ZSJMotor::yloc_available(int loc) {
    if (loc > _ymax)
        return _ymax;
    else if (loc < _ymin)
        return _ymin;
    return loc;
}

// 归零
void ZSJMotor::reset(XYACTION action) {
    // 初始化命令
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_RESET);
    if (action == YONLY)
        x2y(cmd);
    checksum(cmd);
    send(cmd);

    _xdest = 0;
    _xloc = 0;
    emit signal_location_complete(0, int(XONLY));
    emit signal_destination_update(0, int(XONLY));

    if (action == XYBOTH) {
        char cmdy[9];
        cmd_base(cmdy, CMD_ACTION_RESET);
        x2y(cmdy);
        checksum(cmdy);
        send(cmdy);

        _ydest = 0;
        _yloc = 0;
        emit signal_location_complete(0, int(YONLY));
        emit signal_destination_update(0, int(YONLY));
    }
}

// 当前位置
void ZSJMotor::location(XYACTION action) {
    // 初始化命令
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_LOCATION);
    checksum(cmd);
    send(cmd);

    // 构建y轴指令
    char cmdy[9];
    cmd_base(cmdy, CMD_ACTION_LOCATION);
    x2y(cmdy);
    checksum(cmdy);
    send(cmdy);
}

// 移动
void ZSJMotor::move(int dest, XYACTION action) {
    if (action == XYBOTH)
        return move(dest, dest);

    // 构建命令
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_MOVE);
    if (action == YONLY)
        x2y(cmd);
    int2char(&(cmd[4]), dest);
    checksum(cmd);

    // 发送指令
    send(cmd);

    // 更新记录坐标
    if (action == XONLY) {
        _xdest = dest;
        _xworking = true;
        emit signal_destination_update(dest, int(XONLY));

    } else if (action == YONLY) {
        _ydest = dest;
        _yworking = true;
        emit signal_destination_update(dest, int(YONLY));

    }
}
void ZSJMotor::move(int x, int y) {
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_MOVE);
    int2char(&(cmd[4]), x);
    checksum(cmd);

    // 构建y轴指令
    char cmdy[9];
    cmd_base(cmdy, CMD_ACTION_MOVE);
    x2y(cmdy);
    int2char(&(cmdy[4]), y);
    checksum(cmdy);

    // 发送指令
    send(cmd);
    send(cmdy);

    // 更新记录坐标
    _xdest = x; _ydest = y;
    _xworking = true; _yworking = true;
    emit signal_destination_update(x, 0);
    emit signal_destination_update(y, 1);
}

// 等待电机完成工作
//bool ZSJMotor::wait_until_idle(int timeout) {
//    for (int i = 0; i < timeout / MOTOR_WAITE_INTERVAL; ++i) {
//        if (!_xworking && !_yworking)
//            return true;
//        QThread::sleep(100);
//    }
//    return false;
//}
//bool ZSJMotor::wait_until_x_idle(int timeout) {
//    for (int i = 0; i < timeout / MOTOR_WAITE_INTERVAL; ++i) {
//        if (!_xworking)
//            return true;
//        QThread::sleep(100);
//    }
//    return false;
//}
//bool ZSJMotor::wait_until_y_idle(int timeout) {
//    for (int i = 0; i < timeout / MOTOR_WAITE_INTERVAL; ++i) {
//        if (!_yworking)
//            return true;
//        QThread::sleep(100);
//    }
//    return false;
//}

// 设置导程
void ZSJMotor::set_lead(int lead) {
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_LEAD);
    int2char(&(cmd[4]), lead);
    checksum(cmd);
    send(cmd);
}

// 设置细分
void ZSJMotor::set_sub_driver(int sub) {
    // 判断sub是否为2的幂
    // sub的二进制表示只有1位为1，sub-1则直接逆转，因此 &与操作 会得到0
    if ((sub & (sub - 1)) != 0)
        return;

    char cmd[9];
    cmd_base(cmd, CMD_ACTION_SUBDRIVER);
    int2char(&(cmd[4]), sub);
    checksum(cmd);
    send(cmd);
}

// 设置步进角， int step以10e-6为单位角度，如1.8°对应step为1800000
void ZSJMotor::set_step_angle(int step) {
    char cmd[9];
    cmd_base(cmd, CMD_ACTION_STEPANGLE);
    int2char(&(cmd[4]), step);
    checksum(cmd);
    send(cmd);
}

// 发送串口信息
void ZSJMotor::send(const char *msg, int len) {
    if (_status == STATUS_NOT_CONNECTED)
        return;
    QByteArray buf(msg, 9);
    emit signal_write_msg(buf);
}

void ZSJMotor::slot_handle_comm_status(int status) {
    model_status(MotorStatus(status));
    emit signal_motor_status_change(status);
}

// 串口的回调函数
void ZSJMotor::slot_handle_comm_receive(QByteArray buf) {
    char action = buf[2];
    if (action == CMD_ACTION_RESET) {
        // x重置
        qDebug("motor response: x reset success");
        _xloc = 0;
        emit signal_reset_complete(0);

    } else if (action == CMD_ACTION_RESET + CMD_XY_GAP) {
        // y重置
        qDebug("motor response: y reset success");
        _yloc = 0;
        emit signal_reset_complete(1);

    } else if (action == CMD_ACTION_LOCATION) {
        int loc = buf.mid(4, 4).toInt();
        _xloc = loc;
        qDebug("motor response: location: x: %d", loc);
        emit signal_location_complete(loc, 0);

    } else if (action == CMD_ACTION_LOCATION + CMD_XY_GAP) {
        int loc = buf.mid(4, 4).toInt();
        _yloc = loc;
        qDebug("motor response: location: y: %d", loc);
        emit signal_location_complete(loc, 1);

    } else if (action == CMD_ACTION_MOVE) {
//                int loc = buf.mid(4, 4).toInt();
        int loc = char2int32(buf.data());
        _xloc = loc;
        _xworking = false;
        qDebug("motor response: move: x: %d", loc);
        emit signal_move_complete(loc, 0);

    } else if (action == CMD_ACTION_MOVE + CMD_XY_GAP) {
//                int loc = buf.mid(4, 4).toInt();
        int loc = char2int32(buf.data());
        _yloc = loc;
        _yworking = false;
        qDebug("motor response: move: y: %d", loc);
        emit signal_move_complete(loc, 1);

    } else if (action == CMD_ACTION_LEAD) {
        int lead = buf.mid(4, 4).toInt();
        qDebug("motor response: lead: %d", lead);
        emit signal_lead_update(lead);

    } else if (action == CMD_ACTION_SUBDRIVER) {
        int sub = buf.mid(4, 4).toInt();
        qDebug("motor response: action: %d", action);
        emit signal_lead_update(sub);

    } else if (action == CMD_ACTION_STEPANGLE) {
        int step = buf.mid(4, 4).toInt();
        qDebug("motor response: step: %d", step);
        emit signal_lead_update(step);

    } else {
        qDebug("unknown serial response: %x, %s", action, buf);
    }
}

// 构造函数
MotorCommunication::MotorCommunication(QObject *parent) : QObject(parent) {}
// 析构函数
MotorCommunication::~MotorCommunication() {
    _serial->close();
    delete _serial;
}

// 初始化函数
// 由于QSerialPort 只能在一个线程里工作，因此不能在构造函数里（主线程）new QSerialPort
// 启动子线程后，将init函数绑定到QThread::startd()信号
// 可以做到随着线程启动，自动进行初始化
void MotorCommunication::init() {
    _serial = new QSerialPort;
    _serial->setBaudRate(115200);
    connect(_serial, SIGNAL(readyRead()), this, SLOT(slot_handle_serial_received()));
}

// 打开 / 关闭 串口
void MotorCommunication::slot_connect(QString port) {
    _serial->setPortName(port);
    bool succ = _serial->open(QIODevice::ReadWrite);
    if (succ) {
        emit signal_comm_status_change(int(ZSJMotor::STATUS_OK));
    } else {
        emit signal_comm_status_change(int(ZSJMotor::STATUS_FAILED));
    }
}
void MotorCommunication::slot_disconnect() {
    _serial->close();
}

// 接收信号并传输
void MotorCommunication::slot_send(QByteArray buf) {
    bool succ = _serial->write(buf);
    if (!succ) {
        qDebug("motor send failed: %s", qPrintable(QByteArray2hex(buf)));
        emit signal_comm_status_change(ZSJMotor::ERR_PARAMS_ERROR);
    }
}

// 接收串口返回，按bytes数拼接，并逐条emit给控制器
void MotorCommunication::slot_handle_serial_received() {
    QByteArray data = _serial->readAll();
//    qDebug("length: %d", data.size());
    int lines = data.size() / 9;
    for (int i = 0; i < lines; ++i) {
        QByteArray buf = data.mid(i * 9, 9);
        if (!buf.isEmpty()) {
            emit signal_comm_received(buf);
        }
    }
}
