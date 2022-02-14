#ifndef SIMPLEIMAGECHANGER_H
#define SIMPLEIMAGECHANGER_H

#include <QObject>
#include <QVariantAnimation>
#include "graphicresources/independentpixmap.h"

namespace ConnectWindow {

// Provides smooth simple image change
class SimpleImageChanger : public QObject
{
    Q_OBJECT
public:
    explicit SimpleImageChanger(QObject *parent, int animationDuration);

    QPixmap *currentPixmap();
    void setImage(QSharedPointer<IndependentPixmap> image, bool bWithAnimation);

signals:
    void updated();

private slots:
    void onOpacityChanged(const QVariant &value);
    void updatePixmap();

private:
    QPixmap pixmap_;
    int animationDuration_;
    QVariantAnimation opacityAnimation_;
    qreal opacityCurImage_;
    qreal opacityPrevImage_;

    QSharedPointer<IndependentPixmap> curImage_;
    QSharedPointer<IndependentPixmap> prevImage_;

};

} //namespace ConnectWindow

#endif // SIMPLEIMAGECHANGER_H
