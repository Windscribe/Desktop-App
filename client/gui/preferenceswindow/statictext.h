#pragma once

#include <QGraphicsObject>
#include <QRectF>
#include "commongraphics/baseitem.h"

namespace PreferencesWindow {

class StaticText : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit StaticText(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCaption(const QString &caption);
    void setText(const QString &text);

    void updateScaling() override;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QString caption_;
    QString text_;
    bool isCaptionElided_ = false;
    bool isTextElided_ = false;
    QRectF captionRect_;
    QRectF textRect_;
};

} // namespace PreferencesWindow
