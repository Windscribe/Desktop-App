#pragma once

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
    int fullHeight() const;

    BaseItem *item(int index);
    int indexOf(BaseItem *item);
    void addItem(BaseItem *item, int index = -1);
    void removeItem(BaseItem *itemToRemove);
    virtual void clearItems();
    void setItems(const QList<BaseItem *> &items, int startIdx, int endIdx);

    void setSpacerHeight(int height);
    void setIndent(int indent);
    void setFirstItemOffsetY(int offset);
    void setEndSpacing(int offset);

    virtual void hideOpenPopups();
    void updateScaling() override;

    virtual bool hasItemWithFocus();
    virtual QString caption() const;

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
    int firstItemOffset_;
    int endSpacing_;
    int indent_;
    QList<BaseItem *> items_;
};

} // namespace CommonGraphics
