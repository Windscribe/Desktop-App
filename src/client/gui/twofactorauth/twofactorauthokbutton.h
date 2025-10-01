#pragma once

#include <QColor>
#include <QFont>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QVariantAnimation>

#include "commongraphics/clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"

namespace TwoFactorAuthWindow {

class TwoFactorAuthOkButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    enum BUTTON_TYPE { BUTTON_TYPE_ADD, BUTTON_TYPE_LOGIN };

    TwoFactorAuthOkButton(ScalableGraphicsObject *parent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setClickable(bool clickable) override;
    void setClickableHoverable(bool clickable, bool hoverable) override;

    BUTTON_TYPE buttonType() const { return buttonType_; }
    void setButtonType(BUTTON_TYPE type);
    void setFont(const FontDescr &fontDescr);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onColorChanged(const QVariant &value);

private:
    void updateTargetFillColor();

    BUTTON_TYPE buttonType_;
    int width_;
    int height_;
    int arcWidth_;
    int arcHeight_;
    FontDescr fontDescr_;
    QString text_;
    QColor targetFillColor_;
    QColor currentFillColor_;
    double currentfillOpacity_;
    QPropertyAnimation fillColorAnimation_;

    static constexpr int OK_BUTTON_WIDTH = 78;
    static constexpr int OK_BUTTON_HEIGHT = 32;
};

}  // namespace TwoFactorAuthWindow
