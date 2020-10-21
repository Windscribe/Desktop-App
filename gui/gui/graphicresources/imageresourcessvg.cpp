#include "imageresourcessvg.h"

#include <QDirIterator>
#include <QSvgRenderer>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include "utils/logger.h"
#include "dpiscalemanager.h"
#include "utils/widgetutils.h"

ImageResourcesSvg::ImageResourcesSvg() : QThread(nullptr), bNeedFinish_(false), bFininishedGracefully_(false), mutex_(QMutex::Recursive)
{
}

ImageResourcesSvg::~ImageResourcesSvg()
{
    Q_ASSERT(bFininishedGracefully_);
}

void ImageResourcesSvg::clearHash()
{
    for (auto it = hashIndependent_.begin(); it != hashIndependent_.end(); ++it)
    {
        delete it.value();
    }
    hashIndependent_.clear();

    for (auto it = iconHashes_.begin(); it != iconHashes_.end(); ++it)
    {
        delete it.value();
    }
    iconHashes_.clear();
}


void ImageResourcesSvg::clearHashAndStartPreloading()
{
    bNeedFinish_ = true;
    wait();
    clearHash();
    bNeedFinish_ = false;
    start(Priority::LowestPriority);
}

void ImageResourcesSvg::finishGracefully()
{
    bNeedFinish_ = true;
    wait();
    clearHash();
    bFininishedGracefully_ = true;
}

// get pixmap with original size
IndependentPixmap *ImageResourcesSvg::getIndependentPixmap(const QString &name)
{
    QMutexLocker locker(&mutex_);
    auto it = hashIndependent_.find(name);
    if (it != hashIndependent_.end())
    {
        return it.value();
    }
    else
    {
        if (loadFromResource(name))
        {
            return hashIndependent_.find(name).value();
        }
        else
        {
            Q_ASSERT(false);
            return nullptr;
        }
    }
}

IndependentPixmap *ImageResourcesSvg::getIconIndependentPixmap(const QString &name)
{
    QMutexLocker locker(&mutex_);
    auto it = iconHashes_.find(name);
    if (it != iconHashes_.end())
    {
        return it.value();
    }
    else
    {
        if (loadIconFromResource(name))
        {
            return iconHashes_.find(name).value();
        }
        else
        {
            // qCDebug(LOG_BASIC) << "Failed to load Icon: " << name;
            // Q_ASSERT(false);
            return nullptr;
        }
    }
}

IndependentPixmap *ImageResourcesSvg::getFlag(const QString &flagName)
{
    QMutexLocker locker(&mutex_);
    IndependentPixmap *ret = getIndependentPixmap("flags/" + flagName);
    if (ret)
    {
        return ret;
    }
    else
    {
        return getIndependentPixmap("flags/NoFlag");
    }
}

IndependentPixmap *ImageResourcesSvg::getScaledFlag(const QString &flagName, int width, int height)
{
    QMutexLocker locker(&mutex_);
    IndependentPixmap *ret = getIndependentPixmapScaled("flags/" + flagName, width, height);
    if (ret)
    {
        return ret;
    }
    else
    {
        return getIndependentPixmapScaled("flags/NoFlag", width, height);
    }
}

void ImageResourcesSvg::run()
{
    QDirIterator it(":/svg", QDirIterator::Subdirectories);
    while (!bNeedFinish_ && it.hasNext())
    {
        if (it.fileInfo().isFile())
        {
            QMutexLocker locker(&mutex_);
            QString name = it.fileInfo().filePath().mid(6, it.fileInfo().filePath().length() - 10);
            loadFromResource(name);
        }
        it.next();
    }
    qCDebug(LOG_BASIC) << "ImageResourcesSvg::run() - all SVGs loaded";
}

bool ImageResourcesSvg::loadIconFromResource(const QString &name)
{
    if (QFile::exists(name))
    {
        QPixmap *p = WidgetUtils::extractProgramIcon(name);
        if (p)
        {
            iconHashes_[name] = new IndependentPixmap(p);
            return true;
        }
    }

    return false;
}

bool ImageResourcesSvg::loadFromResource(const QString &name)
{
    QSvgRenderer render(":/svg/" + name + ".svg");
    if (!render.isValid())
    {
        return false;
    }
    QPixmap *pixmap = new QPixmap(render.defaultSize() * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap->fill(Qt::transparent);
    QPainter painter(pixmap);
    render.render(&painter);

    pixmap->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    hashIndependent_[name] = new IndependentPixmap(pixmap);
    return true;
}

bool ImageResourcesSvg::loadFromResourceWithCustomSize(const QString &name, int width, int height)
{
    QSvgRenderer render(":/svg/" + name + ".svg");
    if (!render.isValid())
    {
        return false;
    }
    QPixmap *pixmap = new QPixmap(QSize(width, height) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap->fill(Qt::transparent);
    QPainter painter(pixmap);
    render.render(&painter);

    pixmap->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    hashIndependent_[name + "_" + QString::number(width) + "_" + QString::number(height)] = new IndependentPixmap(pixmap);
    return true;
}

IndependentPixmap *ImageResourcesSvg::getIndependentPixmapScaled(const QString &name, int width, int height)
{
    QString modifiedName = name + "_" + QString::number(width) + "_" + QString::number(height);
    auto it = hashIndependent_.find(modifiedName);
    if (it != hashIndependent_.end())
    {
        return it.value();
    }
    else
    {
        if (loadFromResourceWithCustomSize(name, width, height))
        {
            return hashIndependent_.find(modifiedName).value();
        }
        else
        {
            //Q_ASSERT(false);
            return nullptr;
        }
    }
}
