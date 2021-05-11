#ifndef BACKGROUNDSETTINGSITEM_H
#define BACKGROUNDSETTINGSITEM_H

#include "../backend/preferences/preferences.h"
#include "../baseitem.h"
#include "../comboboxitem.h"
#include "selectimageitem.h"
#include "../dividerline.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class BackgroundSettingsItem : public BaseItem
{
    Q_OBJECT
public:
    explicit BackgroundSettingsItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setBackgroundSettings(const ProtoTypes::BackgroundSettings &settings);
    //void setFirewallMode(const ProtoTypes::FirewallSettings &fm);
    //void setFirewallBlock(bool isFirewallBlocked);

    void updateScaling() override;
signals:
    void backgroundSettingsChanged(const ProtoTypes::BackgroundSettings &settings);
    //void firewallModeChanged(const ProtoTypes::FirewallSettings &fm);
    void buttonHoverEnter();
    void buttonHoverLeave();

private slots:
    void onBackgroundModeChanged(QVariant v);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 174 + 24;

    ComboBoxItem *comboBoxMode_;
    SelectImageItem *imageItemDisconnected_;
    SelectImageItem *imageItemConnected_;

    //ComboBoxItem *comboBoxFirewallWhen_;
    QVariantAnimation expandEnimation_;
    bool isExpanded_;
    qreal curDescrTextOpacity_;

    ProtoTypes::BackgroundSettings curBackgroundSettings_;

    DividerLine *dividerLine_;

};

} // namespace PreferencesWindow

#endif // BACKGROUNDSETTINGSITEM_H
