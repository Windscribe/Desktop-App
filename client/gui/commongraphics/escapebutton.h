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
    void onLanguageChanged();

signals:
    void clicked();

private:
    IconButton *iconButton_;
    TEXT_POSITION textPosition_;

    static constexpr int BUTTON_SIZE = 24;
};

} // namespace CommonGraphics
