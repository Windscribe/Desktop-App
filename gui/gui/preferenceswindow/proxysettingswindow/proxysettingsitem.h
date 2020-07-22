#ifndef PROXYSETTINGSITEM_H
#define PROXYSETTINGSITEM_H

#include "../baseitem.h"
#include "../Backend/Preferences/preferences.h"
#include "../comboboxitem.h"
#include "../editboxitem.h"
#include "../dividerline.h"
#include "IPC/generated_proto/types.pb.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class ProxySettingsItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ProxySettingsItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setProxySettings(const ProtoTypes::ProxySettings &ps);

    void updateScaling() override;
signals:
    void proxySettingsChanged(const ProtoTypes::ProxySettings &ps);

private slots:
    void onProxyTypeChanged(QVariant v);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onAddressChanged(const QString &text);
    void onPortChanged(const QString &text);
    void onUsernameChanged(const QString &text);
    void onPasswordChanged(const QString &text);

    void onLanguageChanged() override;

protected:
    void hideOpenPopups() override;

private:
    const int COLLAPSED_HEIGHT = 93;
    const int EXPANDED_HEIGHT = 93 + 43 * 4;

    ComboBoxItem *comboBoxProxyType_;
    QVariantAnimation expandEnimation_;
    bool isExpanded_;
    int curUnscaledHeight_;
    ProtoTypes::ProxySettings curProxySettings_;
    EditBoxItem *editBoxAddress_;
    EditBoxItem *editBoxPort_;
    EditBoxItem *editBoxUsername_;
    EditBoxItem *editBoxPassword_;
    DividerLine *dividerLine_;

    void updatePositions();

};

} // namespace PreferencesWindow

#endif // PROXYSETTINGSITEM_H
