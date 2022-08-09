#ifndef APIRESOLUTIONITEM_H
#define APIRESOLUTIONITEM_H

#include "../baseitem.h"
#include "../connectionwindow/automanualswitchitem.h"

#include <QVariantAnimation>
#include "../editboxitem.h"
#include "types/dnsresolutionsettings.h"

namespace PreferencesWindow {

class ApiResolutionItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ApiResolutionItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setApiResolution(const types::DnsResolutionSettings &dns);

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void apiResolutionChanged(const types::DnsResolutionSettings &dns);

private slots:
    void onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state);
    void onIPChanged(const QString &text);
    void onExpandAnimationValueChanged(const QVariant &value);

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43;

    AutoManualSwitchItem *switchItem_;
    EditBoxItem *editBoxIP_;

    types::DnsResolutionSettings curApiResolution_;


    QVariantAnimation expandEnimation_;
    bool isExpanded_;

    void updatePositions();

};

} // namespace PreferencesWindow

#endif // APIRESOLUTIONITEM_H
