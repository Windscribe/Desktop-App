#ifndef SELECTABLELOCATIONITEMWIDGET_H
#define SELECTABLELOCATIONITEMWIDGET_H

#include <QAbstractButton>
#include "types/locationid.h"

namespace GuiLocations {

class SelectableLocationItemWidget : public QAbstractButton
{
    Q_OBJECT
public:
    explicit SelectableLocationItemWidget(QWidget * parent = nullptr) : QAbstractButton(parent) { }
    enum SelectableLocationItemWidgetType
    {
        REGION,
        CITY
    };

    virtual const LocationID getId() const = 0;
    virtual const QString name() const = 0;
    virtual SelectableLocationItemWidgetType type() = 0;
    virtual void setSelected(bool select) = 0;
    virtual bool isSelected() const = 0;
    virtual bool containsCursor() const = 0;

signals:
    void selected(SelectableLocationItemWidgetType*);
    void clicked(SelectableLocationItemWidgetType*);
};

}
#endif // SELECTABLELOCATIONITEMWIDGET_H
