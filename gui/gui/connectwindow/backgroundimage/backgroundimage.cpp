#include "backgroundimage.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include <QTimer>


namespace ConnectWindow {

BackgroundImage::BackgroundImage(QObject *parent, Preferences *preferences) : QObject(parent), preferences_(preferences), disconnectedImage_(nullptr),
    connectedImage_(nullptr), imageChanger_(this), isConnected_(false)
{
    connect(&imageChanger_, SIGNAL(updated()), SIGNAL(updated()));
    connect(preferences_, SIGNAL(backgroundSettingsChanged(const ProtoTypes::BackgroundSettings &)), SLOT(onBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &)));
    onBackgroundSettingsChanged(preferences_->backgroundSettings());
}

BackgroundImage::~BackgroundImage()
{
}

QPixmap *BackgroundImage::currentPixmap()
{
    return imageChanger_.currentPixmap();
}

void BackgroundImage::changeFlag(const QString &countryCode)
{
    if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_COUNTRY_FLAGS)
    {
        if (countryCode_ != countryCode)
        {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                   countryCode, WIDTH * G_SCALE, 176 * G_SCALE);
            imageChanger_.setImage(indPix, true);
        }
        else if (countryCode_.isEmpty())
        {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                   countryCode, WIDTH * G_SCALE, 176 * G_SCALE);
            imageChanger_.setImage(indPix, false);
        }
    }
    countryCode_ = countryCode;
}

void BackgroundImage::setIsConnected(bool isConnected)
{
    if (isConnected_ != isConnected)
    {
        isConnected_ = isConnected;
        if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_CUSTOM)
        {
            if (isConnected_)
            {
                imageChanger_.setImage(safeGetConnectedImage(), true);
            }
            else
            {
                imageChanger_.setImage(safeGetDisconnectedImage(), true);
            }
        }
    }
}

void BackgroundImage::onBackgroundSettingsChanged(const ProtoTypes::BackgroundSettings &backgroundSettings)
{
    curBackgroundSettings_ = backgroundSettings;
    QTimer::singleShot(0, this, SLOT(handleBackgroundsChange()));
}

void BackgroundImage::handleBackgroundsChange()
{
    if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_CUSTOM)
    {
        QString disconnectedPath = QString::fromStdString(curBackgroundSettings_.background_image_disconnected());
        QString connectedPath = QString::fromStdString(curBackgroundSettings_.background_image_connected());

        if (disconnectedPath.isEmpty())
        {
            disconnectedImage_.reset();
        }
        else
        {
            QPixmap pixmap;
            if (!pixmap.load(disconnectedPath))
            {
                disconnectedImage_.reset();
            }
            else
            {
                pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                pixmap = pixmap.scaled(WIDTH * G_SCALE, 176 * G_SCALE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                disconnectedImage_ = QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
            }
        }

        if (connectedPath.isEmpty())
        {
            connectedImage_.reset();
        }
        else
        {
            QPixmap pixmap;
            if (!pixmap.load(connectedPath))
            {
                connectedImage_.reset();
            }
            else
            {
                pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                pixmap = pixmap.scaled(WIDTH * G_SCALE, 176 * G_SCALE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                connectedImage_ = QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
            }
        }

        if (!isConnected_)
        {
            imageChanger_.setImage(safeGetDisconnectedImage(), false);
        }
        else
        {
            imageChanger_.setImage(safeGetConnectedImage(), false);
        }
    }
    else if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_NONE)
    {
        QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
            "noflag", WIDTH * G_SCALE, 176 * G_SCALE);
        imageChanger_.setImage(indPix, false);
    }
    else if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_COUNTRY_FLAGS)
    {
        QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
            countryCode_, WIDTH * G_SCALE, 176 * G_SCALE);
        imageChanger_.setImage(indPix, false);
    }
    else
    {
        Q_ASSERT(false);
    }
}

QSharedPointer<IndependentPixmap> BackgroundImage::safeGetDisconnectedImage()
{
    if (disconnectedImage_)
    {
        return disconnectedImage_;
    }
    else
    {
        return ImageResourcesSvg::instance().getScaledFlag(
                    "noflag", WIDTH * G_SCALE, 176 * G_SCALE);
    }
}

QSharedPointer<IndependentPixmap> BackgroundImage::safeGetConnectedImage()
{
    if (connectedImage_)
    {
        return connectedImage_;
    }
    else
    {
        return ImageResourcesSvg::instance().getScaledFlag(
                    "noflag", WIDTH * G_SCALE, 176 * G_SCALE);
    }
}


/*void BackgroundImage::updatePixmap()
{
    SAFE_DELETE(pixmap_);

    pixmap_ = new QPixmap(WIDTH * G_SCALE, 176 * G_SCALE);
    pixmap_->fill(Qt::transparent);

    {
        QPainter p(pixmap_);

        if (curBackgroundSettings_.background_type() == ProtoTypes::BackgroundType::BACKGROUND_TYPE_COUNTRY_FLAGS)
        {
            if (!prevCountryCode_.isEmpty())
            {
                QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                    prevCountryCode_, WIDTH * G_SCALE, 176 * G_SCALE);
                p.setOpacity(opacityPrevFlag_);
                if (indPix)
                    indPix->draw(0, 0, &p);
            }

            if (!countryCode_.isEmpty())
            {
                QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                    countryCode_, WIDTH * G_SCALE, 176 * G_SCALE);
                p.setOpacity(opacityCurFlag_);
                if (indPix)
                    indPix->draw(0, 0, &p);
            }
        }
        else
        {
            QSharedPointer<IndependentPixmap> indPix = ImageResourcesSvg::instance().getScaledFlag(
                "noflag", WIDTH * G_SCALE, 176 * G_SCALE);
            p.setOpacity(opacityCurFlag_);
            if (indPix)
                indPix->draw(0, 0, &p);
        }
        /*IndependentPixmap *indBackground = backgroundImage_.getDisconnectedImage();
        if (indBackground)
        {
            p.setOpacity(opacityCurFlag_);
            indBackground->draw(0, 0, &p);
        }
    }
    emit updated();
}*/



} //namespace ConnectWindow
