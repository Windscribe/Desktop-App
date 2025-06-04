#pragma once

#include <QObject>
#include <QVariantAnimation>
#include <QMovie>
#include "../../graphicresources/independentpixmap.h"
#include "types/enums.h"

namespace ConnectWindow {

// Provides smooth image change and gif animation
class ImageChanger : public QObject
{
    Q_OBJECT
public:
    explicit ImageChanger(QObject *parent, int animationDuration);
    virtual ~ImageChanger();

    QPixmap *currentPixmap();
    void setImage(QSharedPointer<IndependentPixmap> pixmap, bool bShowPrevChangeAnimation);
    void setMovie(QSharedPointer<QMovie> movie, bool bShowPrevChangeAnimation);
    void setAspectRatioMode(ASPECT_RATIO_MODE mode) { aspectRatioMode_ = mode; }

signals:
    void updated();

private slots:
    void onOpacityChanged(const QVariant &value);
    void onOpacityFinished();
    void updatePixmap();
    void onMainWindowIsActiveChanged(bool isActive);

private:
    static constexpr int WIDTH = 350;

    QPixmap *pixmap_;
    qreal opacityCurImage_;
    qreal opacityPrevImage_;
    int animationDuration_;
    QVariantAnimation opacityAnimation_;

    struct ImageInfo
    {
        bool isMovie;
        QSharedPointer<IndependentPixmap> pixmap;
        QSharedPointer<QMovie> movie;

        ImageInfo() : isMovie(false) {}
        bool isValid() const
        {
            if (!isMovie)
            {
                return pixmap != nullptr;
            }
            else
            {
                return movie != nullptr;
            }
        }

        void clear(QObject *parent)
        {
            if (isMovie && movie && parent)
            {
                movie->disconnect(parent);
                movie->stop();
            }

            pixmap.reset();
            movie.reset();
            isMovie = false;
        }
    };

    ImageInfo curImage_;
    ImageInfo prevImage_;

    QPixmap customGradient_;
    ASPECT_RATIO_MODE aspectRatioMode_ = ASPECT_RATIO_MODE_STRETCH;
};

} //namespace ConnectWindow
