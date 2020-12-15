#include "imageresourcesjpg.h"

#include <QDirIterator>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include "dpiscalemanager.h"
#include "utils/widgetutils.h"

ImageResourcesJpg::ImageResourcesJpg()
{
    QDirIterator it(":/jpg", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        if (it.fileInfo().isFile())
        {
            QString filename = it.fileInfo().filePath().mid(6, it.fileInfo().filePath().length() - 10);
            QString name;
            int width, height;
            if (parseFileName(filename, name, width, height))
            {
                filenames_[name].push_back( {filename, width, height} ); // FilenameDescr init
                std::sort(filenames_[name].begin(), filenames_[name].end(), sortSizesFunction);
            }
        }
    }
}

ImageResourcesJpg::~ImageResourcesJpg()
{
    clearHash();
}

void ImageResourcesJpg::clearHash()
{
    for (auto it = hashIndependent_.begin(); it != hashIndependent_.end(); ++it)
    {
        delete it.value();
    }
    hashIndependent_.clear();
}


// get pixmap with custom size
IndependentPixmap *ImageResourcesJpg::getIndependentPixmap(const QString &name, int width, int height)
{
    QString uniqName = name + "_" + QString::number(width) + "_" + QString::number(height);
    auto it = hashIndependent_.find(uniqName);
    if (it != hashIndependent_.end())
    {
        return it.value();
    }
    else
    {
        if (loadFromResourceWithCustomSize(name, width, height))
        {
            return hashIndependent_.find(uniqName).value();
        }
        else
        {
            Q_ASSERT(false);
            return nullptr;
        }
    }
}

bool ImageResourcesJpg::loadFromResourceWithCustomSize(const QString &name, int width, int height)
{
    // choosing the best image size
    auto it = filenames_.find(name);
    if (it == filenames_.end())
    {
        return false;
    }

    FilenameDescr fd = it.value().last();
    for (QVector<FilenameDescr>::iterator vectorIt = it.value().begin(); vectorIt != it.value().end(); ++vectorIt)
    {
        if (width*height*DpiScaleManager::instance().curDevicePixelRatio() <= vectorIt->width*vectorIt->height)
        {
            fd = *vectorIt;
            break;
        }
    }

    QImage img(":/jpg/" + fd.filename + ".jpg");
    if (img.isNull())
    {
        return false;
    }

    QPixmap *pixmap = new QPixmap(QSize(width, height) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap->fill(Qt::transparent);
    QPainter painter(pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawImage(QRect(0, 0, pixmap->width(), pixmap->height()), img, QRect(0, 0, img.width(), img.height()));
    pixmap->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    hashIndependent_[name + "_" + QString::number(width) + "_" + QString::number(height)] = new IndependentPixmap(pixmap);
    return true;
}

bool ImageResourcesJpg::parseFileName(const QString &filename, QString &outName, int &outWidth, int &outHeight)
{
    int indDefis = filename.lastIndexOf('_');
    if (indDefis == -1)
    {
        Q_ASSERT(false);
        return false;
    }
    outName = filename.mid(0, indDefis);

    int ind1 = filename.indexOf('x', indDefis);
    if (ind1 == -1)
    {
        Q_ASSERT(false);
        return false;
    }

    QString widthStr = filename.mid(indDefis + 1, ind1 - indDefis - 1);
    bool bOk;
    outWidth = widthStr.toInt(&bOk);
    if (!bOk)
    {
        Q_ASSERT(false);
        return false;
    }

    QString heightStr = filename.mid(ind1 + 1, filename.size() - ind1 - 1);
    outHeight = heightStr.toInt(&bOk);
    if (!bOk)
    {
        Q_ASSERT(false);
        return false;
    }

    return true;
}

bool ImageResourcesJpg::sortSizesFunction(const ImageResourcesJpg::FilenameDescr &f1, const ImageResourcesJpg::FilenameDescr &f2)
{
    return f1.width*f1.height < f2.width*f2.height;
}
