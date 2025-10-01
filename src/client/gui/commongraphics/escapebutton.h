#pragma once

#include <QGraphicsObject>
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"

namespace CommonGraphics {

class EscapeButton : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    enum TEXT_POSITION {
        TEXT_POSITION_BOTTOM = 0,
        TEXT_POSITION_LEFT = 1
    };

    explicit EscapeButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setTextPosition(TEXT_POSITION pos);
    void setClickable(bool isClickable);
    void updateScaling() override;
    int getSize() const { return BUTTON_SIZE; }

public slots:
    void onTextOpacityChanged(const QVariant &value);
    void onLanguageChanged();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

signals:
    void clicked();

private:
    IconButton *iconButton_;
    TEXT_POSITION textPosition_;
    QVariantAnimation textOpacityAnim_;
    double currentOpacity_;
    bool isClickable_;

    static constexpr int BUTTON_SIZE = 32;

    void hover();
};

} // namespace CommonGraphics
