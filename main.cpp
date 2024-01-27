#include "mainwindow.h"

#include <QApplication>
#include "modules/optotrash.h"

// 自定义日志输出格式
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
     QByteArray localMsg = msg.toLocal8Bit();
     const char *file = context.file ? context.file : "";
     file = &(file[6]);
//     const char *function = context.function ? context.function : "";
     switch (type) {
     case QtDebugMsg:
         fprintf(stderr, "\033[32mDebug: %s  \033[35m[%s:%u]\n", localMsg.constData(), file, context.line);
         break;
     case QtInfoMsg:
         fprintf(stderr, "\033[36mInfo: %s  \033[35m[%s:%u]\n", localMsg.constData(), file, context.line);
         break;
     case QtWarningMsg:
         fprintf(stderr, "\033[33mWarning: %s  \033[35m[%s:%u]\n", localMsg.constData(), file, context.line);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "\033[31mCritical: %s  \033[35m[%s:%u]\n", localMsg.constData(), file, context.line);
         break;
     case QtFatalMsg:
         fprintf(stderr, "\033[32mFatal: %s  \033[35m[%s:%u]\n", localMsg.constData(), file, context.line);
         break;
     }
}

int main(int argc, char *argv[])
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    // 配置日志输出格式
    qInstallMessageHandler(myMessageOutput);

    QApplication a(argc, argv);
    ModeMainWindow w;
    w.show();

//    MainWindow w;
//    w.show();
    return a.exec();
}
