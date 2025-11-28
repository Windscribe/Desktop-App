#include "imageresourcespng.h"

#include <QDirIterator>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include "utils/ws_assert.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "dpiscalemanager.h"
#include "widgetutils/widgetutils.h"

ImageResourcesPng::ImageResourcesPng()
{
}

ImageResourcesPng::~ImageResourcesPng()
{
    clearHash();
}

void ImageResourcesPng::clearHash()
{
    hashIndependent_.clear();
}


// get pixmap with custom size
QSharedPointer<IndependentPixmap> ImageResourcesPng::getIndependentPixmap(const QString &name)
{
    auto it = hashIndependent_.find(name);
    if (it != hashIndependent_.end()) {
        return it.value();
    } else {
        if (loadFromResource(name)) {
            return hashIndependent_.find(name).value();
        } else {
            WS_ASSERT(false);
            return nullptr;
        }
    }
}

bool ImageResourcesPng::loadFromResource(const QString &name)
{
    QString filename = ":/png/" + name + ".png";
    if (!QFile::exists(filename)) {
        return false;
    }
    QPixmap pixmap(filename);
    hashIndependent_[name] = QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
    return true;
}