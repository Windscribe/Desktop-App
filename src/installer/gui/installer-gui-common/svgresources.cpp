#include "svgresources.h"

#include <QApplication>
#include <QScreen>
#include <QSvgRenderer>

QPixmap SvgResources::pixmap(const QString &path)
{
    QSvgRenderer render(path);
    if (!render.isValid()) {
        return QPixmap();
    }

    QPixmap p(render.defaultSize()*qApp->primaryScreen()->devicePixelRatio());
    p.fill(Qt::transparent);
    QPainter painter(&p);
    render.render(&painter);
    p.setDevicePixelRatio(qApp->primaryScreen()->devicePixelRatio());

    return p;
}

QIcon SvgResources::icon(const QString &path)
{
    return QIcon(pixmap(path));
}