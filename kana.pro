 QT       += \
    core gui\
    serialport\
    charts\

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# qcustomplot 需要添加这行否则编译报错 unsolved externals
greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport


CONFIG += c++11

# 禁用编号为 c4828 的编译警告
# c4828 是某些文件的编码格式不符合 65001即utf-8 因此会疯狂弹出警告影响qtcreator本身流畅度
# 说的就是你 minsvision
QMAKE_CXXFLAGS += /wd4828

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 解决编译中文乱码
msvc::QMAKE_CFLAGS+= -execution-charset:utf-8
msvc::QMAKE_CXXFLAGS+= -execution-charset:utf-8

# 取消所有debug信息
# DEFINES += QT_NO_DEBUG_OUTPUT

#
# 在项目根目录下的 frameworks/ 新建相关sdk的目录，把所有东西都放到该目录下。
#

# 添加依赖 FTD2XX
INCLUDEPATH += $$PWD/frameworks/fdti/include
LIBS += -L$$PWD/frameworks/fdti -lftd2xx

# 添加依赖 MinSVision
INCLUDEPATH += $$PWD/frameworks/minsvision/include
LIBS += -L$$PWD/frameworks/minsvision -lMVCAMSDK #-lMVCAMSDK_X64

# 添加依赖 opencv
# opencv 包过大，没有放在frameworks目录下，也不进入git版本管理
# 重新配置QT项目需要另行添加opencv包依赖
# 若不区分debug与release（文件名带d） 编译有可能报错：错误 LNK2019 无法解析的外部符号
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/lib/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/lib/ -lopencv_world454d
else:unix: LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/lib/ -lopencv_world454
INCLUDEPATH += $$PWD/frameworks/opencv/build/include
DEPENDPATH += $$PWD/frameworks/opencv/build/include
# 添加bin/opencv_world454.dll，否则会报错找不到该dll文件；也可以将其装到全局
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/bin/ -lopencv_world454
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/bin/ -lopencv_world454d
else:unix: LIBS += -L$$PWD/frameworks/opencv/build/x64/vc15/bin/ -lopencv_world454

# 添加依赖optosky
INCLUDEPATH += $$PWD/frameworks/optosky/include
LIBS += -L$$PWD/frameworks/optosky -lDriver

# 添加依赖qcustomplot
#INCLUDEPATH += $$PWD/frameworks/qcustomplot


# 尽量不要手动修改，用qtcreator生成
SOURCES += \
    analyse/measure.cpp \
    camera/capturethread.cpp \
    camera/ftdiwrap.cpp \
    camera/minsvision.cpp \
    main.cpp \
    mainwindow.cpp \
    modules/charutils.cpp \
    modules/modbus.cpp \
    modules/optotrash.cpp \
    modules/zsjmotor.cpp \
    views/plotwidget.cpp \
    views/qcustomplot.cpp \
    views/beamprofilerdlg.cpp \
    views/chart.cpp \
    views/modemainwindow.cpp \
    views/serialportdlg.cpp \
    views/versiondlg.cpp

HEADERS += \
    analyse/measure.h \
    camera/capturethread.h \
    camera/ftdiwrap.h \
    camera/minsvision.h \
    camera/types.h \
    mainwindow.h \
    modules/charutils.h \
    modules/modbus.h \
    modules/optotrash.h \
    modules/zsjmotor.h \
    views/plotwidget.h \
    views/qcustomplot.h \
    views/beamprofilerdlg.h \
    views/chart.h \
    views/modemainwindow.h \
    views/serialportdlg.h \
    views/versiondlg.h

FORMS += \
    mainwindow.ui \
    views/beamprofilerdlg.ui \
    views/modemainwindow.ui \
    views/plotwidget.ui \
    views/serialportdlg.ui \
    views/versiondlg.ui

## Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

#LIBS += -L$$PWD/../../opencv/build/x64/vc15/lib/ -lopencv_world454
#INCLUDEPATH += $$PWD/../../opencv/build/include
