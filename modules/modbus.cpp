#include "modbus.h"

#include "modules/charutils.h"
#include <QDateTime>
#include <QTime>

// ModBus 通讯协议 简化版
// 详见 "佳信智能配置软件ModbusV40_20231122" 具体发送的串口指令
//
// 1 bytes 从机
const char MODBUS_SLAVE         = 0x40;
// 1 bytes 功能码
const char MODBUS_FUNCTION      = 0x00;
// 2 bytes 寄存器地址 不同的寄存器地址 对应 不同的指令
const char MODBUS_REGISTER[2]   = {0x00,0x00};
// 2 bytes 寄存器数量
const char MODBUS_REGISTER_N[2] = {0x00,0x00};
// 1 bytes 数据量
const char MODBUS_N_BYTES       = 0x00;
// n bytes 数据 对应 MODBUS_N_BYTES 个 bytes
const char MODBUS_DATA[]        = {0x00};
// 2 bytes CRC校验
const char MODBUS_CRC_CHECK[2]  = {0x00,0x00};

// ModBus 基本构成

// 功能码
// 读寄存器
const char MODBUS_FUNC_READ_REGISTER = 0x03;
// 写 单个 寄存器
const char MODBUS_FUNC_WRITE_SINGLE  = 0x06;
// 写 多个 寄存器
const char MODBUS_FUNC_WRITE_MULTI   = 0x10;
// 错误码
const char MODBUS_FUNC_ERROR         = 0x90;

// 寄存器地址
// 每个指令的寄存器为：基础地址 + 通道数 * 100 + 偏移
// 例如 CH1 的重置命令对应寄存器地址为：10100 + 1 * 100 + 10
//
// 寄存器 基础地址
const int CMD_INT_REGISTER_BASE = 10100;
// 每个通道 100个 寄存器
const int CMD_INT_CHANNEL       = 100;
// 对应的hex
const char CMD_REGISTER_BASE[2] = {0x27,0x74};
const char CMD_CHANNEL          = 0x64;
const char CMD_CH0[2]           = {0x00,0x00};
const char CMD_CH1[2]           = {0x00,0x64};
const char CMD_CH2[2]           = {0x00,0xc8};
const char CMD_CH3[2]           = {0x01,0x2c};

// 指令 即 寄存器偏移
// 这里只写了一部分
const int CMD_ACTION_INT_STATE       = 0;
const int CMD_ACTION_INT_LOCATION    = 2;
const int CMD_ACTION_INT_RESET       = 10;
const int CMD_ACTION_INT_STOP        = 12;
const int CMD_ACTION_INT_MOVE_REL    = 14;
const int CMD_ACTION_INT_MOVE_ABS    = 16;
const int CMD_ACTION_INT_VELOCITY    = 20;
const int CMD_ACTION_INT_SUBDIVISION = 50;
// 对应的hex
const char CMD_ACTION_STATE       = 0x00;
const char CMD_ACTION_LOCATION    = 0x02;
const char CMD_ACTION_RESET       = 0x0a;
const char CMD_ACTION_STOP        = 0x0c;
const char CMD_ACTION_MOVE_REL    = 0x0e;
const char CMD_ACTION_MOVE_ABS    = 0x10;
const char CMD_ACTION_VELOCITY    = 0x14;
const char CMD_ACTION_SUBDIVISION = 0x32;

// 指令长度
//
// 读取 指令长度
const int CMD_N_BYTES_READ   = 8;
// 写单个寄存器 指令长度
const int CMD_N_BYTES_SINGLE = 8;
// 写多个寄存器 指令长度
const int CMD_N_BYTES_MULTI_2  = 13;
const int CMD_N_BYTES_MULTI_4  = 17;
const int CMD_N_BYTES_MULTI_8  = 25;
// 写多个 基本长度
const int CMD_N_BYTES_MULTI_BASE = 9;

// 固定指令
// 4个channel的原点
const char CMD_CH0_RESET[13] = {0x40,0x10,0x27,0x7E,0x00,0x02,0x04,0x00,0x00,0x00,0x00,0x9B,0xCB};
const char CMD_CH1_RESET[13] = {0x40,0x10,0x27,0xE2,0x00,0x02,0x04,0x00,0x00,0x00,0x00,0x92,0xF2};
const char CMD_CH2_RESET[13] = {0x40,0x10,0x28,0x46,0x00,0x02,0x04,0x00,0x00,0x00,0x00,0xD8,0x89};
const char CMD_CH3_RESET[13] = {0x40,0x10,0x28,0xAA,0x00,0x02,0x04,0x00,0x00,0x00,0x00,0xD6,0x94};

// 多轴读取状态
const char CMD_MULTI_STATE[8] = {0x40,0x03,0x2E,0xE0,0x00,0x1E,0xC2,0x0D};

// 状态一共 4 bytes
// 目前看最后 1 byte 即可满足大部分常见
// e.g. 40 03 0C 00 00 00 21 00 00 00 00 00 00 00 00 86 11
// 00 00 00 21 是状态位，其中最后 1 byte 0x21 按bit展开
//     0       0       1       0         0       0       0       1
//   bit7    bit6    bit5    bit4      bit3    bit2    bit1    bit0
//           负向  执行原点故障         脉冲输出   回原点

// 根据指令 获取 数据长度
int n_register_by_action(int action) {
    if (action == CMD_ACTION_RESET || action == CMD_ACTION_STOP ||
            action == CMD_ACTION_MOVE_REL || action == CMD_ACTION_MOVE_ABS)
        return 2;

    if (action == CMD_ACTION_VELOCITY)
        return 8;

    if (action == CMD_ACTION_STATE)
        return 6;
}

// 构建基本命令：读取寄存器
QByteArray cmd_read(int action, int channel) {
    QByteArray cmd;
    cmd.resize(CMD_N_BYTES_READ);
    // 从站地址
    cmd[0] = MODBUS_SLAVE;
    // 功能码
    cmd[1] = MODBUS_FUNC_READ_REGISTER;
    // 寄存器地址
    int32_t register_ = CMD_INT_REGISTER_BASE + channel * CMD_INT_CHANNEL + action;
    cmd[2] = *((char *)(&register_) + 1);
    cmd[3] = *((char *)(&register_) + 0);
    // 寄存器数量
    cmd[4] = 0x00;
    int n_register = n_register_by_action(action);
    cmd[5] = *((char *)(&n_register) + 0);
    // crc校验
    int2char(&(cmd.data()[6]), calculateCRC((unsigned char*)(cmd.data()), 6), 2);
    return cmd;
}

// 构建基本命令：写入多个寄存器
QByteArray cmd_write_multi(int action, int channel) {
    // 寄存器 数目
    int n_register = n_register_by_action(action);
    // 数据 数目
    int n_data = 2 * n_register;
    QByteArray cmd;
    cmd.resize(CMD_N_BYTES_MULTI_BASE + n_data);

    // 从站地址
    cmd[0] = MODBUS_SLAVE;
    // 功能码
    cmd[1] = MODBUS_FUNC_WRITE_MULTI;
    // 寄存器地址
    int32_t register_ = CMD_INT_REGISTER_BASE + channel * CMD_INT_CHANNEL + action;
    cmd[2] = *((char *)(&register_) + 1);
    cmd[3] = *((char *)(&register_) + 0);
    // 寄存器数量
    cmd[4] = 0x00;
    cmd[5] = *((char *)(&n_register) + 0);
    // 数据量
    cmd[6] = *((char *)(&n_data) + 0);
    // 数据
    for (int i = 0; i < n_data; ++i) {
        cmd[7 + i] = 0x00;
    }
    // crc校验
    cmd[7 + n_data] = 0x00;
    cmd[8 + n_data] = 0x00;
    return cmd;
}

// 组装移动命令
QByteArray cmd_move(int dest, int channel) {
    QByteArray cmd = cmd_write_multi(CMD_ACTION_INT_MOVE_ABS, channel);
    int2char(&(cmd.data()[7]), dest, 4);
    int2char(&(cmd.data()[11]), calculateCRC((unsigned char*)(cmd.data()), 11), 2);
    return cmd;
}

// 组装停止命令
QByteArray cmd_stop(int channel) {
    QByteArray cmd = cmd_write_multi(CMD_ACTION_INT_STOP, channel);
    int2char(&(cmd.data()[11]), calculateCRC((unsigned char*)(cmd.data()), 11), 2);
    return cmd;
}

// 设置速度
// 示例工具中，每条绝对移动指令前都会设置速度
// 目前先按示例工具中的默认速度设置
QByteArray cmd_velocity(int channel, int v = 32000, int acceleration=200) {
    QByteArray cmd = cmd_write_multi(CMD_ACTION_INT_VELOCITY, channel);

    // 一共 16 bytes 的数据
    // 4 bytes: 运行速度 默认32000
    int2char(&(cmd.data()[7]), v, 4);
    // 4 bytes: 起始速度 默认3200
    int2char(&(cmd.data()[11]), v / 10, 4);
    // 2 bytes: 加速时间 默认200
    int2char(&(cmd.data()[15]), acceleration, 2);
    // 2 bytes: 减速时间 默认200
    int2char(&(cmd.data()[17]), acceleration, 2);
    // 4 bytes: 停止速度 默认1600
    int2char(&(cmd.data()[19]), v / 20, 4);

    int2char(&(cmd.data()[23]), calculateCRC((unsigned char*)(cmd.data()), 23), 2);
    return cmd;
}

// 相对位置移动
// 示例工具中，每条相对移动指令不需要设置速度
QByteArray cmd_move_relative(int dis, int channel) {
    QByteArray cmd = cmd_write_multi(CMD_ACTION_INT_MOVE_REL, channel);
    int2char(&(cmd.data()[7]), dis, 4);
    int2char(&(cmd.data()[11]), calculateCRC((unsigned char*)(cmd.data()), 11), 2);
    return cmd;
}

ModBusComm::ModBusComm(QObject *parent) : QObject(parent),
    _index(-1),
    _xworking(true),
    _yworking(true),
    _xdirection(true),
    _ydirection(true)
{}
ModBusComm::~ModBusComm() {}

// 初始化函数
// 由于QSerialPort 只能在一个线程里工作，因此不能在构造函数里（主线程）new QSerialPort
// 启动子线程后，将init函数绑定到QThread::startd()信号
// 可以做到随着线程启动，自动进行初始化
void ModBusComm::init() {
    _serial_read = QByteArray();
    _serial = new QSerialPort;
    _serial->setBaudRate(115200);
    connect(_serial, SIGNAL(readyRead()), this, SLOT(slot_handle_serial_received()));
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(slot_request_location()));
}

// 打开串口
void ModBusComm::slot_connect(QString port) {
    _serial->setPortName(port);
    bool succ = _serial->open(QIODevice::ReadWrite);
    if (succ) {
        // 重置发送池
        _sendpool.clear();
        // 每次启动写入一个默认速度
        slot_send(cmd_velocity(0));
        slot_send(cmd_velocity(1));
        // 启动位置信息获取timer
        _timer->start(100);

        emit signal_comm_status_change(int(ModBus::OK));
    } else {
        emit signal_comm_status_change(int(ModBus::FAILED));
    }
}
// 关闭串口
void ModBusComm::slot_disconnect() {
    _serial->close();
    _sendpool.clear();

    _timer->stop();
    emit signal_comm_status_change(int(ModBus::NOT_CONNECTED));
}
// 接收 打开 / 关闭 自动更新位置信息
void ModBusComm::slot_auto_location(bool open, int ms) {
    if (open) {
        _timer->start(ms);
    } else {
        _timer->stop();
    }
}
// 获取当前位置
void ModBusComm::slot_request_location() {
    // 发送与slot_send一样，只是少了debug
    QByteArray buf = QByteArray(CMD_MULTI_STATE, 8);
    if (_sendpool.size() == 0) {
        bool succ = _serial->write(buf);
        if (!succ) {
            qWarning("motor send failed: %s", qPrintable(QByteArray2hex(buf)));
            emit signal_comm_status_change(int(ModBus::FAILED));
        }
    }
    _sendpool.push_back(buf);
}
void ModBusComm::slot_acquire_location(int index) {
    QByteArray buf = QByteArray(CMD_MULTI_STATE, 8);
    _index = index;
    slot_send(buf);
}
// 发送指令
void ModBusComm::slot_send(QByteArray buf) {
    if (_sendpool.size() == 0) {
        // 发送池为空，直接发
//        qDebug("motor send: %s", qPrintable(QByteArray2hex(buf)));
        bool succ = _serial->write(buf);
        if (!succ) {
            qWarning("motor send failed: %s", qPrintable(QByteArray2hex(buf)));
            emit signal_comm_status_change(int(ModBus::FAILED));
        }
    }
    // 往发送池里填
    // 收到一次返回，会执行弹出首条指令
    // 然后发送下一条
    _sendpool.push_back(buf);
}
// 接收串口返回，按bytes数拼接，并逐条emit给控制器
void ModBusComm::slot_handle_serial_received() {
    // 读取全部
    _serial_read.append(_serial->readAll());
//    qDebug("receive from serial: %s", qPrintable(char2str(_serial_read.data(), _serial_read.size())));

    // 读取的位置
    int index = 0;
    while (index < _serial_read.size() - 1) {
        // 读取到 从机标志位
        if (_serial_read[index] == MODBUS_SLAVE) {
            // 1 bytes 从机
            index ++;
            // 1 bytes 操作码
            char func = _serial_read[index];
            index ++;

            if (func == MODBUS_FUNC_READ_REGISTER) {
                qint64 ts = QDateTime::currentMSecsSinceEpoch();

                // 读取返回
                int n_bytes = int(_serial_read.at(index));
                // n bytes 数据 + 2 bytes crc校验
                index += n_bytes + 2;

                // 若剩下bytes不足，撤销本次的index偏置，退出循环，等待下次read
                if (index > _serial_read.size()) {
                    _serial_read = _serial_read.mid(index - n_bytes - 2 - 2);
                    return;
                }

                // 获取运动状态
                // 4 bytes 的状态位，目前用最后 1 byte 已经可以判读状态
                unsigned char state0 = _serial_read.data()[index - n_bytes - 2 + 4];
                bool working0 = (state0 & 0x08);
                bool back0 = (state0 & 0x10);
                unsigned char state1 = _serial_read.data()[index - n_bytes - 2 + 4 + 12];
                bool working1 = (state1 & 0x08);
                bool back1 = (state1 & 0x20);
//                qDebug("ch0: working: %d; direction: %d;", working0, back0);
//                qDebug("ch1: working: %d; direction: %d", working1, back1);

                // 若状态改变，发送信号
                if (_xworking != working0 || _yworking != working1) {
                    _xworking = working0;
                    _yworking = working1;
                    emit signal_comm_working_state(_xworking, _yworking);
                }
                // 若方向改变，发送信号
                if (_xdirection != back0 || _ydirection != back1) {
                    _xdirection = back0;
                    _ydirection = back1;
                    emit signal_comm_direction(_xdirection, _ydirection);
                }

                // 获取位置
                int loc0 = char2int32(&(_serial_read.data()[index - n_bytes - 2 + 5]));
                int loc1 = char2int32(&(_serial_read.data()[index - n_bytes - 2 + 5 + 12]));
                emit signal_comm_location_update(loc0, loc1, true, true, ts, _index);

//                qInfo("modbus response: ch%d state: location: %d", 0, loc0);
//                qInfo("modbus response: ch%d state: location: %d", 1, loc1);


            } else if (func == MODBUS_FUNC_WRITE_SINGLE) {
                // 写单个寄存器返回
                qInfo("modbus response: write single register: not implemented");

            } else if (func == MODBUS_FUNC_WRITE_MULTI) {
                // 写多个寄存器返回
                // 获取 通道 和 指令
                int register_int = char2uint16(&(_serial_read.data()[index]));
                int channel = (register_int - CMD_INT_REGISTER_BASE) / 100;
                int cmd = (register_int - CMD_INT_REGISTER_BASE) % 100;

                // 剩下 6 bytes
                // 都是 2 bytes 寄存器地址 + 2 bytes 寄存器数量 + 2 bytes crc校验
                index += 6;
                // 若剩下bytes不足，撤销本次的index偏置，退出循环，等待下次read
                if (index > _serial_read.size()) {
                    _serial_read = _serial_read.mid(index - 6 - 2);
                    return;
                }

                if (cmd == CMD_ACTION_INT_RESET) {
                    // 重置完成
                    qInfo("modbus response: ch%d reset complete", channel);

                } else if (cmd == CMD_ACTION_INT_MOVE_ABS) {
                    // 绝对位置移动完成
                    qInfo("modbus response: ch%d move abs complete", channel);

                } else if (cmd == CMD_ACTION_INT_MOVE_REL) {
                    // 相对位置移动完成
                    qInfo("modbus response: ch%d move relative complete", channel);

                } else if (cmd == CMD_ACTION_INT_STOP) {
                    // 停止运动完成
                    qInfo("modbus response: ch%d stop complete", channel);

                } else if (cmd == CMD_ACTION_INT_VELOCITY) {
                    // 设置速度完成
                    qInfo("modbus response: ch%d write velocity complete", channel);

                } else {
                    // 未知指令返回
                    qWarning("modbus unknown response: ch%d, cmd: %d", channel, cmd);
                }

            } else if (func == MODBUS_FUNC_ERROR) {
                // 错误返回
                // 1 bytes 错误码 + 2 bytes crc校验
                index += 3;
                // 若剩下bytes不足，撤销本次的index偏置，退出循环，等待下次read
                if (index > _serial_read.size()) {
                    _serial_read = _serial_read.mid(index - 3 - 2);
                    return;
                }
                qWarning("receive response: error");

            } else {
                // 未知功能码
                // 直接进行下一次循环
                qWarning("receive unknown function: %d", func);
                continue;
            }

            // 移除发送池首条
            _sendpool.pop_front();

            // 移除后若发送池不为空，直接发送下一条
            if (_sendpool.size() > 0) {
//                qDebug("motor send: %s", qPrintable(QByteArray2hex(_sendpool.first())));
                bool succ = _serial->write(_sendpool.first());
                if (!succ) {
                    qWarning("motor send failed: %s", qPrintable(QByteArray2hex(_sendpool.first())));
                    emit signal_comm_status_change(int(ModBus::FAILED));
                }
            }

            _serial_read.clear();

        } else {
            // 开头不是 MODBUS_SLAVE
            // 移动到下一个byte
            qWarning("unknown serial response: %d/%d", index, _serial_read.size());
            qWarning("receive from serial: %s", qPrintable(char2str(_serial_read.data(), _serial_read.size())));

            index ++;

        }
    }

    // 非 MODBUS_SLAVE 跳出循环
    // 直接重置
//    _serial_read.clear();
}

ModBus::ModBus(QObject *parent) : QObject(parent)
{
    // 异步串口
    _comm = new ModBusComm;
    _comm->moveToThread(&_thread);
    connect(&_thread, SIGNAL(started()), _comm, SLOT(init()));
    connect(&_thread, &QThread::finished, _comm, &QObject::deleteLater);
    connect(this, SIGNAL(signal_write_msg(QByteArray)), _comm, SLOT(slot_send(QByteArray)));
    connect(this, SIGNAL(signal_connect(QString)), _comm, SLOT(slot_connect(QString)));
    connect(this, SIGNAL(signal_disconnect()), _comm, SLOT(slot_disconnect()));
    connect(this, SIGNAL(signal_acquire_location(int)), _comm, SLOT(slot_acquire_location(int)));
    connect(this, SIGNAL(signal_auto_location(bool,int)), _comm, SLOT(slot_auto_location(bool,int)));

    connect(_comm, SIGNAL(signal_comm_status_change(int)), this, SLOT(slot_handle_comm_status(int)));
    connect(_comm, SIGNAL(signal_comm_location_update(int,int,bool,bool,qint64,int)),
            this, SLOT(slot_handle_comm_location_update(int,int,bool,bool,qint64,int)));
    connect(_comm, SIGNAL(signal_comm_reach_origin(int,int)),
            this, SLOT(slot_handle_comm_reach_origin(int,int)));
    connect(_comm, SIGNAL(signal_comm_working_state(bool,bool)), this, SLOT(slot_handle_comm_working_state(bool,bool)));
    connect(_comm, SIGNAL(signal_comm_direction(bool,bool)), this, SLOT(slot_handle_comm_direction(bool,bool)));
    _thread.start();
}
ModBus::~ModBus() {
    // 退出子线程
    _thread.quit();
    _thread.wait();
}

// 收到子线程的状态改变
void ModBus::slot_handle_comm_status(int status) {
    model_status(ModBusStatus(status));
    emit signal_status_change(status);
}

// 修改状态
void ModBus::model_status(ModBusStatus status) {
    if (status == _status) return;
    _status = status;
}
void ModBus::model_status(ModBusStatus status, QString port) {
    if (status == _status && port.compare(_serialport) == 0) return;
    _status = status;
    _serialport = port;
}

// 连接串口，不影响子线程
void ModBus::connect_(QString port) {
    emit signal_connect(port);
}
// 断开串口，不影响子线程
void ModBus::disconnect_() {
    emit signal_disconnect();
    _serialport.clear();
}

void ModBus::reset(int channel) {
    if (channel == 0)
        emit signal_write_msg(QByteArray(CMD_CH0_RESET, CMD_N_BYTES_MULTI_2));
    else if (channel == 1)
        emit signal_write_msg(QByteArray(CMD_CH1_RESET, CMD_N_BYTES_MULTI_2));
}

void ModBus::move(int dest, int channel) {
    emit signal_write_msg(cmd_move(dest, channel));
}

void ModBus::stop(int channel) {
    emit signal_write_msg(cmd_stop(channel));
}

void ModBus::move_relative(int dis, int channel) {
    emit signal_write_msg(cmd_move_relative(dis, channel));
}

void ModBus::velocity(int v, int channel, int acceleration) {
    emit signal_write_msg(cmd_velocity(channel, v, acceleration));
}

void ModBus::subdivision(int v, int channel) {
    QByteArray cmd = cmd_write_multi(CMD_ACTION_INT_VELOCITY, channel);

    // 一共 16 bytes 的数据
    // 4 bytes: 运行速度 默认32000
    int2char(&(cmd.data()[7]), v, 4);
    // 4 bytes: 起始速度 默认3200
    int2char(&(cmd.data()[11]), v / 10, 4);
    // 2 bytes: 加速时间 默认200
    int2char(&(cmd.data()[15]), 200, 2);
    // 2 bytes: 减速时间 默认200
    int2char(&(cmd.data()[17]), 200, 2);
    // 4 bytes: 停止速度 默认1600
    int2char(&(cmd.data()[19]), v / 20, 4);

    int2char(&(cmd.data()[23]), calculateCRC((unsigned char*)(cmd.data()), 23), 2);

    emit signal_write_msg(cmd);
}

