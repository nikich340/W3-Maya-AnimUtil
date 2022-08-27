#include "W3MayaAnimUtil.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    /*"windowsvista"
    "Windows"
    "Fusion"*/
    QApplication::setApplicationName("W3MayaAnimUtil");
    QApplication::setOrganizationName("nikich340");

    QSettings settings;
    QApplication::setStyle( QSettings().value("GUIStyle", "windowsvista").toString() );

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/icon2.ico"));

    W3MayaAnimUtil w;
    w.show();
    return app.exec();
}
