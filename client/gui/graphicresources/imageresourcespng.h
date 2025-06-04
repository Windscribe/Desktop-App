#pragma once

#include <QPixmap>
#include <QMutex>
#include <QHash>
#include "independentpixmap.h"

class ImageResourcesPng
{
public:
    static ImageResourcesPng &instance() {
        static ImageResourcesPng ir;
        return ir;
    }

    void clearHash();

    QSharedPointer<IndependentPixmap> getIndependentPixmap(const QString &name);

private:
    ImageResourcesPng();
    virtual ~ImageResourcesPng();

    QHash<QString, QSharedPointer<IndependentPixmap>> hashIndependent_;

    bool loadFromResource(const QString &name);
};
