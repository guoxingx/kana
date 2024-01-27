#include "optotrash.h"

#include <QDateTime>


// opto的CCD尺寸列表 别问为什么 他sdk里就这么写的
// 可以作为波长和频率的精度，5040对应12位
// 写在文件开头，否则被写在后面的函数调用时会编译错误
int GetCCDSize(QString pn) {
    QString pnFirst = pn.mid(3,1);
    QString num = pn.mid(3, 4);
    QString num5 = pn.mid(3, 5);
    int size = 2048;

    switch (pnFirst.toInt())
    {
        case 1:
            size = 1024;
            if (num == "1010")
                size = 512;
            break;
        case 2:
            size = 2048;
            if (num == "2100")
                size = 512;
            break;
        case 3:
            size = 4096;
            if (num == "3030" || num == "3330")
                size = 2048;
            break;
        case 4:
            size = 3648;
            break;
        case 5:
            if (num == "5000" || num == "5020" || num == "5001" || num == "5330")
                size = 2048;
            else if (num == "5111" || num == "5003")
                size = 2068;
            else if (num == "5520")
                size = 2068;
            else if (num == "5105")
                size = 3100;
            else if (num == "5100")
            {
                /*
                int start = getUsedPixelStart();
                int end = getUsedPixelEnd(2047);

                if (start == -1 || end == -1 || start >= end)
                    ;//throw new Exception($"Pixel Range from:{start} to:{end} invalid, please reset!");

                size = end - start + 1;*/
            }
            else if (num == "5334" || num == "5040" || num == "5034")
                size = 4096;
            break;
        case 6:
            size = 1024;
            break;
        case 8:
            size = 512;
            if (num == "8600")
                size = 256;
            break;
        case 9:
            if (num5.toLower() == "9100d")
                size = 512;
            break;
    }

    return size;
}

OptoTrash::OptoTrash(QObject *parent) : QObject(parent),
    _device_state(Dev_PlugOff),
    m_intergral_time_milli(100),
    m_ccdsize(SPECTRUM_SIZE)
{
    // 创建worker并移动到子线程
    opworker = new OptoWorker;
    opworker->moveToThread(&m_opthread);
    // 连接信号槽
    connect(&m_opthread, &QThread::finished, opworker, &QObject::deleteLater);
    connect(opworker, &OptoWorker::signal_state_change, this, &OptoTrash::slot_handle_device_state_change);
    // 对于protected slot 用这种connect命名方式才能编译通过，否则会报c2248无法访问
    connect(this, SIGNAL(signal_capture_once(int, int)), opworker, SLOT(slot_capture_once(int, int)));
    connect(this, SIGNAL(signal_capture_dark(int)), opworker, SLOT(slot_capture_dark(int)));
    connect(this, SIGNAL(signal_capture_continuous_start(int, int)),
            opworker, SLOT(slot_capture_continuous_start(int, int)));
    connect(this, SIGNAL(signal_capture_continuous_stop()),
            opworker, SLOT(slot_capture_continuous_stop()));

    // 启动子线程
    m_opthread.start();
}

OptoTrash::~OptoTrash() {
    delete m_wavelength;
    delete m_spectrum;
    delete m_spectrum_dark;

    // 退出子线程
    m_opthread.quit();
    m_opthread.wait();
}

// 打开 / 关闭
bool OptoTrash::open() {
    bool res = openSpectraMeter();
    if (res) {
        _device_state = Dev_PlugOff;

        // 初始化一些基本信息
        memset(m_wavelength, 0, sizeof(m_wavelength));
        memset(m_spectrum_dark, 0, sizeof(m_spectrum_dark));
        memset(m_spectrum, 0, sizeof(m_spectrum));

        // 初始化设备信息
        _device_state = Dev_Initializing;
        ErrorFlag flag = initialize();
        if (flag != INIT_SUCCESS) {
            qDebug("spectrum init failed");
            return false;
        }

        // 用产品号获取ccd size和SN码
        u8 dataout[SPECTRUM_SIZE] = {0};
        getProductPN(dataout);
        m_ccdsize = GetCCDSize(QString::fromLocal8Bit((char *)dataout));
        m_SN.fromLocal8Bit((char *)dataout);

        // 获取波长范围
        float *pWavelength = getWavelength();
        memcpy(m_wavelength, pWavelength, SPECTRUM_SIZE * sizeof (SPECTRUM_SIZE));

        // 积分时间精度，校验和 等信息，暂时不用
        // 0 bit: 0 means 2 byte integration time, 1 means 4 byte integration time
        // 1 bit: 0 means integration unit is us, 1means integration time unit is ms
        // 2 bit: 0 means checksum will be checked, 1 means will not check checks
        // getAttribute();

        _device_state = Dev_Idle;
        return true;
    }
    _device_state = Dev_Disconnected;
    return res;
}
bool OptoTrash::close() {
    stop_capture_continuous();
    bool res = closeSpectraMeter();
    if (res)
        _device_state = Dev_PlugIn;
    return res;
}
bool OptoTrash::is_device_open() {
    if (_device_state == NULL || _device_state == Dev_PlugIn || _device_state == Dev_PlugOff)
        return false;
    return true;
}
// 设置积分时间，单位是ms
bool OptoTrash::set_integral_time_milli(int ms) {
    // 这个接口输入的是us 之后再研究
    // setIntegrationTime(ms);

    m_intergral_time_milli = ms;
    return true;
}

// 单次采集
bool OptoTrash::capture_once(int integral_ms, int index) {
    if (_device_state != Dev_Idle)
        return false;
    emit signal_capture_once(integral_ms, index);
    return true;
}
// 同步采集
bool OptoTrash::capture_sync(int integral_ms, int* spectrum, int length) {
    if (_device_state != int(OptoTrash::Dev_Idle))
        return false;

    // 启动采集
    // 这个接口文档里说是us作为单位，但是实际似乎是ms
    getSpectrum(integral_ms);

    // 等待光谱就绪
    while (getSpectrumDataReadyFlag()!= 1)
        QThread::msleep(1);

    // 获取数据
    Spectrumsp data = ReadSpectrum();

    // 复制数据
    memcpy(spectrum, data.array, length * sizeof(int));
    return true;
}

// 采集暗光谱
bool OptoTrash::capture_dark(int integral_ms) {
    qDebug("capture_dark: %d", integral_ms);
    if (_device_state != Dev_Idle)
        return false;
    emit signal_capture_dark(integral_ms);
}

// 启动连续采集
bool OptoTrash::start_capture_continuous(int integral_ms, int interval_ms) {
    emit signal_capture_continuous_start(integral_ms, interval_ms);
    return true;
}
// 关闭连续采集
bool OptoTrash::stop_capture_continuous() {
    emit signal_capture_continuous_stop();
    qDebug("send stop signal to worker");
    return true;
}

// 子线程worker
OptoWorker::OptoWorker(QObject *parent) : QObject(parent),
    _device_state(int(OptoTrash::Dev_Idle)),
    _integral_ms(100)
{
    // 控制连续采样的计时器
    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(slot_capture_once()));
}

OptoWorker::~OptoWorker() {
    _timer->stop();
    delete _timer;
}
void OptoWorker::slot_capture_once(int integral_ms, int index) {
    _integral_ms = integral_ms;
    capture(index);
}
// 提供给连续采集调用的 用设置过的积分时间
void OptoWorker::slot_capture_once() {
    capture();
}
void OptoWorker::slot_capture_dark(int integral_ms) {
    _integral_ms = integral_ms;
    capture_dark();
}
void OptoWorker:: slot_capture_continuous_start(int integral_ms, int interval_ms) {
    _integral_ms = integral_ms;
    // 当 interval_ms=0 时，计时器会持续循环，这个循环是被事件循环控制的
    // 所以在子线程应该用 timer.start(0) 来替代死循环
    _timer->start(interval_ms);
    qDebug("worker start continuous capture");
}
void OptoWorker::slot_capture_continuous_stop() {
    _timer->stop();
    qDebug("worker stop");
}

bool OptoWorker::capture(int index) {
    if (index > -1)
        qDebug("capture: %d", index);

    if (_device_state != int(OptoTrash::Dev_Idle))
        return false;
    model_device_state(OptoTrash::Dev_Running);

    // 启动采集
    // 这个接口文档里说是us作为单位，但是实际似乎是ms
    getSpectrum(_integral_ms);

    // 等待光谱就绪
    while (getSpectrumDataReadyFlag()!= 1)
        QThread::msleep(_ready_flag_ms);
    // 获取时间戳
    qint64 ts = QDateTime::currentMSecsSinceEpoch();

    // 获取数据
    Spectrumsp data = ReadSpectrum();
    model_device_state(OptoTrash::Dev_Idle);

    // 获取成功，则发送信号到控制器线程
    if (data.valid_flag != SPECTRUMDATA_VALID)
        return false;
    emit signal_spectrum_update(data.array, ts, index);
    return true;
}

bool OptoWorker::capture_dark() {
    qDebug("worker capture_dark: %d, %d", _integral_ms, _device_state);
    if (_device_state != int(OptoTrash::Dev_Idle))
        return false;
    model_device_state(OptoTrash::Dev_Running);

    // 这个接口文档里说是us作为单位，但是实际似乎是ms
    bool res = getDarkSpectrum(_integral_ms);
    qDebug("getDarkSpectrum: %d", res);

    // 等待暗光谱就绪
    while (getSpectrumDataReadyFlag()!= 1)
        QThread::msleep(_ready_flag_ms);
    // 获取时间戳
    qint64 ts = QDateTime::currentMSecsSinceEpoch();

    // 获取数据
    Spectrumsp data = ReadSpectrum();
    qDebug("ReadSpectrum: flag: %d", data.valid_flag);
    model_device_state(OptoTrash::Dev_Idle);
    // 获取成功，则发送信号到控制器线程
    if (data.valid_flag != SPECTRUMDATA_VALID)
        // 这里原demo很逆天，采集暗地失败后，居然会采集光谱然后返回？完全不懂奥普到底在干什么
        // capture(integral_ms);
        return false;
    emit signal_spectrum_dark_update(data.array, ts);
    return true;
}
