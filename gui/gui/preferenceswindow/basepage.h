#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QGraphicsObject>
#include "baseitem.h"

namespace PreferencesWindow {

const int PAGE_WIDTH = 282;

class BasePage : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit BasePage(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    int currentPosY() const;
    int currentHeight() const;

    BaseItem *item(int index);
    void addItem(BaseItem *item);
    void removeItem(BaseItem *itemToRemove);

    virtual void hideOpenPopups();
    void updateScaling() override;

    virtual bool hasComboMenuWithFocus();

signals:
    void escape();
    void heightChanged(int newHeight);
    void scrollToPosition(int itemPos);
    void scrollToRect(QRect r);

protected:
    QList<BaseItem *> items() const;

private slots:
    void recalcItemsPos();

private:
    int height_;
    QList<BaseItem *> items_;
};

} // namespace PreferencesWindow

#endif // BASEPAGE_H
