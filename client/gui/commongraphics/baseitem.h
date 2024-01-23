#pragma once

#include <QGraphicsObject>
#include <QVariant>
#include <QVariantAnimation>
#include "commongraphics/commongraphics.h"
#include "commongraphics/clickablegraphicsobject.h"

namespace CommonGraphics {

class BaseItem : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit BaseItem(ScalableGraphicsObject *parent, int height, QString id = "", bool clickable = false, int width = PAGE_WIDTH);
    QRectF boundingRect() const override;

    void setHeight(int height);

    virtual void hideOpenPopups();

    QString id();
    void setId(QString id);

    virtual void hide(bool animate = true);
    virtual void show(bool animate = true);
    bool isHidden();

    virtual bool hasItemWithFocus() { return false; }

signals:
    void heightChanged(int newHeight);
    void hidden();

protected:
    int height_;
    int width_;
    QString id_;

private slots:
    void onHideProgressChanged(const QVariant &value);

private:
    QVariantAnimation hideAnimation_;
    double hideProgress_;
};

} // namespace CommonGraphics
