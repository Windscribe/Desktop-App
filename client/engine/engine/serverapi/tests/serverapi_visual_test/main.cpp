#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QApplication a(argc, argv);

    Widget w;
    w.show();
    int ret = a.exec();
    return ret;
}
