#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/textbutton.h"

namespace CommonGraphics {

class ListButton: public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    enum Style { kText, kBright, kDark };

    explicit ListButton(ScalableGraphicsObject *parent, Style style, const QString &text);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setText(const QString &text);
    void setRadius(int radius);
    void setStyle(Style style);

    void hover();
    void unhover();

    // Bubble buttons only
    void setFillColor(const QColor &color);
    void setTextColor(const QColor &color);


signals:
    void clicked();

private:
    static constexpr int kMinimumWidth = 27;

    Style style_;
    QString text_;

    TextButton *textButton_;
    BubbleButton *bubbleButton_;

    int margin_;

    void updatePositions();
};

} // namespace CommonGraphics