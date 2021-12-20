#ifndef LOCATIONSTRAYMENUITEMDELEGATE_H
#define LOCATIONSTRAYMENUITEMDELEGATE_H

#include <QItemDelegate>

class LocationsTrayMenuItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuItemDelegate(QObject *parent);
    ~LocationsTrayMenuItemDelegate();

    const QFont &getFontForItems() const { return font_; }
    void setFontForItems(const QFont &font);

protected:
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QFont font_;
    int menuHeight_;
};

#endif // LOCATIONSTRAYMENUITEMDELEGATE_H
