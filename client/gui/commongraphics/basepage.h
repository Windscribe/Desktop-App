#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QGraphicsObject>
#include "baseitem.h"
#include "commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

class BasePage : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit BasePage(ScalableGraphicsObject *parent = nullptr, int width = PAGE_WIDTH);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    int currentPosY() const;
    int currentHeight() const;

    BaseItem *item(int index);
    int indexOf(BaseItem *item);
    void addItem(BaseItem *item);
    void removeItem(BaseItem *itemToRemove);
    void clearItems();

    void setSpacerHeight(int height);
    void setIndent(int indent);

    virtual void hideOpenPopups();
    void updateScaling() override;

    virtual bool hasItemWithFocus();

signals:
    void escape();
    void heightChanged(int newHeight);
    void scrollToPosition(int itemPos);

protected:
    QList<BaseItem *> items() const;

private slots:
    void recalcItemsPos();

private:
    int height_;
    int width_;
    int spacerHeight_;
    int indent_;
    QList<BaseItem *> items_;
};

} // namespace CommonGraphics

#endif // BASEPAGE_H
