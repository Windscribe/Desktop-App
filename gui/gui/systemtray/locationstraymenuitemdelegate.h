#ifndef LOCATIONSTRAYMENUITEMDELEGATE_H
#define LOCATIONSTRAYMENUITEMDELEGATE_H

#include <QItemDelegate>

class LocationsTrayMenuItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuItemDelegate(QObject *parent);
    ~LocationsTrayMenuItemDelegate();

    void setFontForItems(const QFont &font);

protected:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;

private:
    QFont font_;
    int menuHeight_;
};

#endif // LOCATIONSTRAYMENUITEMDELEGATE_H
