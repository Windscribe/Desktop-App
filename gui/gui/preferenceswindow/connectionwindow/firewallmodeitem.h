#ifndef FIREWALLMODEITEM_H
#define FIREWALLMODEITEM_H

#include "../Backend/Preferences/preferences.h"
#include "IPC/generated_proto/types.pb.h"
#include "../baseitem.h"
#include "../comboboxitem.h"
#include "../dividerline.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class FirewallModeItem : public BaseItem
{
    Q_OBJECT
public:
    explicit FirewallModeItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setFirewallMode(const ProtoTypes::FirewallSettings &fm);
    void setFirewallBlock(bool isFirewallBlocked);

    void updateScaling() override;
signals:
    void firewallModeChanged(const ProtoTypes::FirewallSettings &fm);

private slots:
    void onFirewallModeChanged(QVariant v);
    void onFirewallWhenChanged(QVariant v);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onLanguageChanged() override;

protected:
    void hideOpenPopups() override;

private:
    const int COLLAPSED_HEIGHT = 50;
    const int EXPANDED_HEIGHT = 131;

    ComboBoxItem *comboBoxFirewallMode_;
    ComboBoxItem *comboBoxFirewallWhen_;
    QVariantAnimation expandEnimation_;
    bool isExpanded_;
    qreal curDescrTextOpacity_;

    ProtoTypes::FirewallSettings curFirewallMode_;

    DividerLine *dividerLine_;

};

} // namespace PreferencesWindow

#endif // FIREWALLMODEITEM_H
