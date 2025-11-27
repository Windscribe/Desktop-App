#pragma once

#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace ConnectWindow {

class IpUtilsMenu : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit IpUtilsMenu(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setIpAddress(const QString &address, bool isPinned = false);
    void setRotateButtonEnabled(bool enabled);

signals:
    void favouriteClick();
    void rotateClick();
    void closeClick();
    void favouriteHoverEnter();
    void favouriteHoverLeave();
    void rotateHoverEnter();
    void rotateHoverLeave();

private:
    static constexpr int kHeight = 24;

    IconButton *favouriteButton_;
    IconButton *rotateButton_;
    IconButton *closeButton_;

    QString currentIpAddress_;
    bool isIpPinned_;

    void updatePositions();
    void updateFavouriteIcon();
};

} // namespace ConnectWindow
