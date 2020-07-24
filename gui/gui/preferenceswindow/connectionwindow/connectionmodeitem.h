#ifndef CONNECTIONMODEITEM_H
#define CONNECTIONMODEITEM_H

#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "ipc/generated_proto/types.pb.h"
#include "../baseitem.h"
#include "automanualswitchitem.h"
#include "../comboboxitem.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class ConnectionModeItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ConnectionModeItem(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);
    void setConnectionMode(const ProtoTypes::ConnectionSettings &cm);

    virtual void updateScaling();
signals:
    void connectionlModeChanged(const ProtoTypes::ConnectionSettings &cm);

private slots:
    void onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state);
    void onFirewallWhenChanged(QVariant v);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onCurrentProtocolItemChanged(QVariant value);
    void onCurrentPortItemChanged(QVariant value);

    void onPortMapChanged();

protected:
    void hideOpenPopups();

private:
    const int COLLAPSED_HEIGHT = 50;
    const int EXPANDED_HEIGHT = 50 + 43 + 43;

    PreferencesHelper *preferencesHelper_;

    AutoManualSwitchItem *switchItem_;
    ComboBoxItem *comboBoxProtocol_;
    ComboBoxItem *comboBoxPort_;

    bool isPortMapInitialized_;

    ProtoTypes::ConnectionSettings curConnectionMode_;

    QVariantAnimation expandEnimation_;
    bool isExpanded_;

    void updateProtocol(ProtoTypes::Protocol protocol);
    void updateConnectionMode();

};

} // namespace PreferencesWindow

#endif // CONNECTIONMODEITEM_H
