#include "W3MayaAnimUtil.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    /*"windowsvista"
    "Windows"
    "Fusion"*/
    QApplication::setApplicationName("W3MayaAnimUtil");
    QApplication::setOrganizationName("nikich340");

    if ( QSettings().contains("GUIStyle") )
        QApplication::setStyle( QSettings().value("GUIStyle", "Fusion").toString() );

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/icon2.ico"));

    W3MayaAnimUtil w;
    w.show();
    return app.exec();
}
