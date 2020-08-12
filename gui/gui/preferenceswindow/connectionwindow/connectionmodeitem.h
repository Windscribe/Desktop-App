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
    enum class ButtonType { PROTOCOL, PORT };
    explicit ConnectionModeItem(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setConnectionMode(const ProtoTypes::ConnectionSettings &cm);
    QPointF getButtonScenePos(ConnectionModeItem::ButtonType type) const;
    bool isPortMapInitialized() const { return isPortMapInitialized_; }

    void updateScaling() override;
signals:
    void connectionlModeChanged(const ProtoTypes::ConnectionSettings &cm);
    void buttonHoverEnter(ConnectionModeItem::ButtonType type);
    void buttonHoverLeave(ConnectionModeItem::ButtonType type);

private slots:
    void onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state);
    void onFirewallWhenChanged(QVariant v);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onCurrentProtocolItemChanged(QVariant value);
    void onCurrentPortItemChanged(QVariant value);

    void onPortMapChanged();

protected:
    void hideOpenPopups() override;

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43 + 43;

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
