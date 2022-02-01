#ifndef MAKECUSTOMSHADOW_H
#define MAKECUSTOMSHADOW_H

#include <QPixmap>

class MakeCustomShadow
{
public:
    static QPixmap makeShadowForPixmap(QPixmap &sourcePixmap, qreal distance, qreal blurRadius, const QColor &color);
};

#endif // MAKECUSTOMSHADOW_H
