#include "imageresourcessvg.h"

#include <QDirIterator>
#include <QSvgRenderer>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QScreen>
#include "utils/ws_assert.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"
#include "dpiscalemanager.h"
#include "widgetutils/widgetutils.h"

ImageResourcesSvg::ImageResourcesSvg() : QThread(nullptr), bNeedFinish_(false), bFininishedGracefully_(false)
{
}

ImageResourcesSvg::~ImageResourcesSvg()
{
    WS_ASSERT(bFininishedGracefully_);
}

void ImageResourcesSvg::clearHash()
{
    hashIndependent_.clear();
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
QSharedPointer<IndependentPixmap> ImageResourcesSvg::getIndependentPixmap(const QString &name)
{
    QMutexLocker locker(&mutex_);
    auto it = hashIndependent_.find(name);
    if (it != hashIndependent_.end()) {
        return it.value();
    } else {
        if (loadFromResource(name)) {
            return hashIndependent_.find(name).value();
        } else {
            //WS_ASSERT(false);
            return nullptr;
        }
    }
}

QSharedPointer<IndependentPixmap> ImageResourcesSvg::getIconIndependentPixmap(const QString &name)
{
    QMutexLocker locker(&mutex_);
    auto it = iconHashes_.find(name);
    if (it != iconHashes_.end()) {
        return it.value();
    } else {
        if (loadIconFromResource(name)) {
            return iconHashes_.find(name).value();
        } else {
            // qCDebug(LOG_BASIC) << "Failed to load Icon: " << name;
            // WS_ASSERT(false);
            return nullptr;
        }
    }
}

QSharedPointer<IndependentPixmap> ImageResourcesSvg::getFlag(const QString &flagName)
{
    QMutexLocker locker(&mutex_);
    QSharedPointer<IndependentPixmap> ret = getIndependentPixmap("flags/" + flagName);
    if (ret) {
        return ret;
    } else {
        return getIndependentPixmap("flags/noflag");
    }
}

QSharedPointer<IndependentPixmap> ImageResourcesSvg::getCircleFlag(const QString &flagName)
{
    QSharedPointer<IndependentPixmap> ret = getIndependentPixmapScaled("flags/" + flagName + "_circle", 24*G_SCALE, 24*G_SCALE, 0);
    if (ret) {
        return ret;
    } else {
        return getIndependentPixmapScaled("flags/noflag", 48*G_SCALE, 24*G_SCALE, IMAGE_FLAG_CIRCLE);
    }
}

QSharedPointer<IndependentPixmap> ImageResourcesSvg::getScaledFlag(const QString &flagName, int width, int height,
                                                    int flags)
{
    QMutexLocker locker(&mutex_);
    QSharedPointer<IndependentPixmap> ret = getIndependentPixmapScaled("flags/" + flagName, width, height, flags);
    if (ret) {
        return ret;
    } else {
        return getIndependentPixmapScaled("flags/noflag", width, height, flags);
    }
}

void ImageResourcesSvg::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    QDirIterator it(":/svg", QDirIterator::Subdirectories);
    while (!bNeedFinish_ && it.hasNext()) {
        it.next();
        if (it.fileInfo().isFile()) {
            QMutexLocker locker(&mutex_);
            QString name = it.fileInfo().filePath().mid(6, it.fileInfo().filePath().length() - 10);
            loadFromResource(name);
        }
    }
    qCDebug(LOG_BASIC) << "ImageResourcesSvg::run() - all SVGs loaded";
}

bool ImageResourcesSvg::loadIconFromResource(const QString &name)
{
    if (QFile::exists(name)) {
        QPixmap p = WidgetUtils::extractProgramIcon(name);
        if (!p.isNull()) {
            iconHashes_[name] = QSharedPointer<IndependentPixmap>(new IndependentPixmap(p));
            return true;
        }
    }
    return false;
}

bool ImageResourcesSvg::loadFromResource(const QString &name)
{
    QString filename = ":/svg/" + name + ".svg";
    if (!QFile::exists(filename)) {
        return false;
    }
    QSvgRenderer render(filename);
    if (!render.isValid()) {
        return false;
    }
    QPixmap pixmap(render.defaultSize() * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    render.render(&painter);

    pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    hashIndependent_[name] = QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
    return true;
}

bool ImageResourcesSvg::loadFromResourceWithCustomSize(const QString &name, int width, int height,
                                                       int flags)
{
    QString filename = ":/svg/" + name + ".svg";
    if (!QFile::exists(filename)) {
        return false;
    }
    QSvgRenderer render(filename);
    if (!render.isValid()) {
        return false;
    }
    const auto sizeScale = DpiScaleManager::instance().curDevicePixelRatio();
    QSize realSize(width * sizeScale, height * sizeScale);
    if (flags & IMAGE_FLAG_SQUARE) {
        if (realSize.width() > realSize.height())
            realSize.setHeight(realSize.width());
        else
            realSize.setWidth(realSize.height());
    }
    QPixmap pixmap(realSize);
    pixmap.fill(Qt::transparent);
    {
        QPixmap temp(realSize);
        temp.fill(Qt::transparent);
        QRectF rc(0, 0, width * sizeScale, height * sizeScale);
        rc.moveTo((realSize.width() - rc.width()) / 2, (realSize.height() - rc.height()) / 2);
        QPainter painter(&temp);
        if (flags & IMAGE_FLAG_CIRCLE) {
            QPainterPath path;
            // NB: using "height" as width here to make a circle
            path.addEllipse(rc.width()/2 - rc.height()/2, 0, rc.height(), rc.height());
            painter.setClipPath(path);
        }
        render.render(&painter, rc);
        if (flags & IMAGE_FLAG_CIRCLE) {
            pixmap = temp.copy(QRect(pixmap.width()/2 - pixmap.height()/2, 0, pixmap.height(), pixmap.height()));
        } else {
            pixmap = temp;
        }
    }
    if (flags & IMAGE_FLAG_GRAYED) {
        auto image = pixmap.toImage();
        for (int i = 0; i < image.height(); ++i) {
            auto *scanline = reinterpret_cast<QRgb*>(image.scanLine(i));
            for (int j = 0; j < image.width(); ++j) {
                const auto gray = qGray(scanline[j]);
                const auto alpha = qAlpha(scanline[j]);
                scanline[j] = QColor(gray, gray, gray, alpha).lighter(200).rgba();
            }
        }
        pixmap = QPixmap::fromImage(image);
    }
    pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());

    QString modifiedName = name + "_" + QString::number(width) + "_" + QString::number(height);
    if (flags) modifiedName += "_" + QString::number(flags);
    hashIndependent_[modifiedName] = QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
    return true;
}

QSharedPointer<IndependentPixmap> ImageResourcesSvg::getIndependentPixmapScaled(const QString &name, int width,
                                                                 int height, int flags)
{
    QString modifiedName = name + "_" + QString::number(width) + "_" + QString::number(height);
    if (flags) modifiedName += "_" + QString::number(flags);
    auto it = hashIndependent_.find(modifiedName);
    if (it != hashIndependent_.end()) {
        return it.value();
    } else {
        if (loadFromResourceWithCustomSize(name, width, height, flags)) {
            return hashIndependent_.find(modifiedName).value();
        } else {
            //WS_ASSERT(false);
            return nullptr;
        }
    }
}
