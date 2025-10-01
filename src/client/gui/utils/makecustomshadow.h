#pragma once

#include <QPixmap>

class MakeCustomShadow
{
public:
    static QPixmap makeShadowForPixmap(QPixmap &sourcePixmap, qreal distance, qreal blurRadius, const QColor &color);
};
