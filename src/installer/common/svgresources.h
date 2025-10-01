#pragma once

#include <QIcon>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QString>

class SvgResources : public QObject
{
    Q_OBJECT
public:
    static SvgResources &instance()
    {
        static SvgResources res;
        return res;
    }

    QPixmap pixmap(const QString &path);
    QIcon icon(const QString &path);
};