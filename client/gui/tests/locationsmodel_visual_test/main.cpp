#include "widget.h"

#include <QApplication>

#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    QApplication a(argc, argv);

    Q_INIT_RESOURCE(engine);
    Q_INIT_RESOURCE(gif);
    Q_INIT_RESOURCE(jpg);
    Q_INIT_RESOURCE(svg);
    Q_INIT_RESOURCE(windscribe);

    DpiScaleManager::instance();    // init dpi scale manager
    ImageResourcesSvg::instance().clearHashAndStartPreloading();

    Widget w;
    w.show();

    int ret = a.exec();
    ImageResourcesSvg::instance().finishGracefully();
    return ret;
}
