QT       += \
    core gui\
    serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 解决编译中文乱码
msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
#msvc:QMAKE_CXXFLAGS += -execution-charset:utf-8

# 取消所有打印信息
# DEFINES += QT_NO_DEBUG_OUTPUT

#
# 在项目根目录下的 frameworks/ 新建相关sdk的目录，把所有东西都放到该目录下。
#

# 添加依赖 MinSVision
INCLUDEPATH += $$PWD/frameworks/minsvision/include
LIBS += -L$$PWD/frameworks/minsvision -lMVCAMSDK #-lMVCAMSDK_X64


# 尽量不要手动修改，用qtcreator生成
SOURCES += \
    analyse/measure.cpp \
    camera/capturethread.cpp \
    camera/minsvision.cpp \
    main.cpp \
    mainwindow.cpp \
    views/serialportdlg.cpp \
    views/versiondlg.cpp

HEADERS += \
    analyse/measure.h \
    camera/capturethread.h \
    camera/minsvision.h \
    camera/types.h \
    mainwindow.h \
    views/serialportdlg.h \
    views/versiondlg.h

FORMS += \
    mainwindow.ui \
    views/serialportdlg.ui \
    views/versiondlg.ui

## Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
