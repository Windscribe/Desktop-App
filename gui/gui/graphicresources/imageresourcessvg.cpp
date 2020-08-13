#include "imageresourcessvg.h"

#include <QDirIterator>
#include <QSvgRenderer>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include "utils/logger.h"
#include "dpiscalemanager.h"
#include "utils/widgetutils.h"

ImageResourcesSvg::ImageResourcesSvg()
{
}

ImageResourcesSvg::~ImageResourcesSvg()
{
    clearHashes();
    clearHash();
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



// get pixmap with original size
IndependentPixmap *ImageResourcesSvg::getIndependentPixmap(const QString &name)
{
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
            //Q_ASSERT(false);
            return nullptr;
        }
    }
}

IndependentPixmap *ImageResourcesSvg::getIconIndependentPixmap(const QString &name)
{
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

void ImageResourcesSvg::clearHashes()
{
    for (auto it = hash_.begin(); it != hash_.end(); ++it)
    {
        delete it.value();
    }
    hash_.clear();

    for (auto it = scaledHashes_.begin(); it != scaledHashes_.end(); ++it)
    {
        for (auto inner = it.value().begin(); inner != it.value().end(); ++inner)
        {
            delete inner.value();
        }
    }
    scaledHashes_.clear();
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
    QDirIterator it(":/svg", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        if (it.fileInfo().isFile())
        {
            QString n = it.fileInfo().filePath().mid(6, it.fileInfo().filePath().length() - 10);
            if (name == n)
            {
                QSvgRenderer render(it.fileInfo().filePath());
                QPixmap *pixmap = new QPixmap(render.defaultSize() * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio());
                pixmap->fill(Qt::transparent);
                QPainter painter(pixmap);
                render.render(&painter);

                pixmap->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                hashIndependent_[name] = new IndependentPixmap(pixmap);
                return true;
            }
        }
        it.next();
    }
    return false;
}

bool ImageResourcesSvg::loadFromResourceWithCustomSize(const QString &name, int width, int height)
{
    QDirIterator it(":/svg", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        if (it.fileInfo().isFile())
        {
            QString n = it.fileInfo().filePath().mid(6, it.fileInfo().filePath().length() - 10);
            if (name == n)
            {
                QSvgRenderer render(it.fileInfo().filePath());
                QPixmap *pixmap = new QPixmap(QSize(width, height) * DpiScaleManager::instance().curDevicePixelRatio());
                pixmap->fill(Qt::transparent);
                QPainter painter(pixmap);
                render.render(&painter);

                pixmap->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                hashIndependent_[name + "_" + QString::number(width) + "_" + QString::number(height)] = new IndependentPixmap(pixmap);
                return true;
            }
        }
        it.next();
    }
    return false;
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
