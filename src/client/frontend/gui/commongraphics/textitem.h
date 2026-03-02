#pragma once

#include <QGraphicsObject>
#include <QFont>
#include <QColor>
#include "scalablegraphicsobject.h"
#include "graphicresources/fontdescr.h"

namespace CommonGraphics {

class TextItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit TextItem(ScalableGraphicsObject *parent, const QString &text, const FontDescr &fd, QColor color, int maxWidth);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

    void setText(const QString &text);
    void setMaxWidth(int maxWidth);

    int getHeight() const;

private:
    void recalcBoundingRect();
    QFont getFont() const;

    QString text_;
    FontDescr fontDescr_;
    QColor color_;
    int width_;
    int height_;
    int maxWidth_;
};

}
