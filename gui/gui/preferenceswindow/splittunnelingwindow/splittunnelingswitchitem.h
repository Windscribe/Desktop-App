#ifndef SPLITTUNNELINGSWITCHITEM_H
#define SPLITTUNNELINGSWITCHITEM_H

#include <QTimer>
#include "../baseitem.h"
#include "../checkboxitem.h"
#include "../comboboxitem.h"
#include "../ConnectionWindow/subpageitem.h"
#include "CommonGraphics/iconbutton.h"
#include "IPC/generated_proto/types.pb.h"

#include "Tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class SplitTunnelingSwitchItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingSwitchItem(ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setSettings(ProtoTypes::SplitTunnelingSettings settings);
    void setClickable(bool clickable);

    void setAppsCount(int count);
    void setIpsAndHostnamesCount(int count);

    virtual void updateScaling();

signals:
    void settingsChanged(ProtoTypes::SplitTunnelingSettings settings);
    void appsPageClick();
    void ipsAndHostnamesPageClick();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onActiveSwitchChanged(bool checked);
    void onExpandAnimationValueChanged(QVariant value);
    void onCurrentModeChanged(QVariant value);
    void onModeInfoHoverEnter();
    void onModeInfoHoverLeave();

private:
    ProtoTypes::SplitTunnelingSettings settings_;
    CheckBoxItem *activeCheckBox_;
    ComboBoxItem *modeComboBox_;
    SubPageItem *appsSubPageItem_;
    SubPageItem *ipsAndHostnamesSubPageItem_;

    IconButton *modeInfo_;

    QVariantAnimation expandEnimation_;

    const int COLLAPSED_HEIGHT = 43;
    const int EXPANDED_HEIGHT = COLLAPSED_HEIGHT + 43 + 43 + 43;

    void reloadModeComboBox();
    void updateActiveUI(bool checked);
    void updateModeUI(ProtoTypes::SplitTunnelingMode mode);

    void updatePositions();

};

}
#endif // SPLITTUNNELINGSWITCHITEM_H
