#include "W3MayaAnimUtil.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/icon2.ico"));

    W3MayaAnimUtil w;
    w.show();
    return app.exec();
}
