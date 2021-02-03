#ifndef SELECTABLELOCATIONITEMWIDGET_H
#define SELECTABLELOCATIONITEMWIDGET_H

#include <QAbstractButton>
#include "types/locationid.h"

namespace GuiLocations {

class IItemWidget : public QAbstractButton
{
    Q_OBJECT
public:
    explicit IItemWidget(QWidget * parent = nullptr) : QAbstractButton(parent) { }
    enum ItemWidgetType
    {
        REGION,
        CITY
    };

    virtual bool isExpanded() const = 0;
    virtual bool isForbidden() const = 0;
    virtual bool isDisabled() const = 0;
    virtual const LocationID getId() const = 0;
    virtual const QString name() const = 0;
    virtual ItemWidgetType type() = 0;
    virtual void setSelectable(bool selectable) = 0;
    virtual void setAccented(bool select) = 0;
    virtual bool isAccented() const = 0;
    virtual bool containsCursor() const = 0;
    virtual bool containsGlobalPoint(const QPoint &pt) = 0;
    virtual QRect globalGeometry() const = 0;

signals:
    void accented(ItemWidgetType*);
    void clicked(ItemWidgetType*);
};

}
#endif // SELECTABLELOCATIONITEMWIDGET_H
