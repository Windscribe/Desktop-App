#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/bubblebuttonbright.h"
#include "commongraphics/textbutton.h"

namespace ProtocolWindow {

class ProtocolListButton: public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ProtocolListButton(ScalableGraphicsObject *parent, const QString &text, bool textOnly, const QColor &textColor = QColor(255, 255, 255, 128));
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

signals:
    void clicked();

private:
    QString text_;

    CommonGraphics::TextButton *textButton_;
    CommonGraphics::BubbleButtonBright *bubbleButton_;

    int margin_;

    void updatePositions();
};

} // namespace ProtocolWindow