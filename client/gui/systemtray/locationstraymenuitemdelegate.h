#pragma once

#include <QItemDelegate>

class LocationsTrayMenuItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuItemDelegate(QObject *parent, double scale);

    const QFont &getFontForItems() const { return font_; }
    void setFontForItems(const QFont &font);

    int calcWidth(const QModelIndex & index) const;

protected:
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QFont font_;
    double scale_;
    const int CITY_CAPTION_MAX_WIDTH = 210;
};
