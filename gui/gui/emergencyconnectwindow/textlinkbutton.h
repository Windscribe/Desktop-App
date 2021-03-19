#ifndef TEXTLINKBUTTON_H
#define TEXTLINKBUTTON_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include <QFont>
#include <QTextDocument>
#include "commongraphics/clickablegraphicsobject.h"

namespace EmergencyConnectWindow {

class TextLinkButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit TextLinkButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void animateShow(int animationSpeed);
    void animateHide(int animationSpeed);

    void updateScaling() override;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    void onLanguageChanged();

private slots:
    void onLinkOpacityChanged(const QVariant &value);
    void onTextOpacityChanged(const QVariant &value);

private:
    int width_;
    int height_;

    static constexpr int LINK_TEXT_HEIGHT = 15;

    double curLinkOpacity_;
    double curTextOpacity_;

    QVariantAnimation linkOpacityAnimation_;
    QVariantAnimation textOpacityAnimation_;

    void recalculateDimensions();


};

}

#endif // TEXTLINKBUTTON_H
