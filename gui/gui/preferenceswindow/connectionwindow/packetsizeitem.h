#ifndef PACKETSIZEITEM_H
#define PACKETSIZEITEM_H

#include <QTimer>
#include "../baseitem.h"
#include "../backend/preferences/preferences.h"
#include "automanualswitchitem.h"

#include <QVariantAnimation>
#include "msseditboxitem.h"
#include "ipc/generated_proto/types.pb.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class PacketSizeItem : public BaseItem
{
    Q_OBJECT
public:
    explicit PacketSizeItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setPacketSize(const ProtoTypes::PacketSize &ps);
    void setPacketSizeDetectionState(bool on);

    void updateScaling() override;

signals:
    void packetSizeChanged(const ProtoTypes::PacketSize &ps);
    void detectPacketMssButtonClicked();
    void showTooltip(TooltipInfo data);
    void hideTooltip(TooltipId id);

private slots:
    void onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state);
    void onMssChanged(const QString &text);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onAutoDetectAndGenerateHoverEnter();
    void onAutoDetectAndGenerateHoverLeave();

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43;

    AutoManualSwitchItem *switchItem_;
    MssEditBoxItem *editBoxMSS_;

    ProtoTypes::PacketSize curPacketSize_;

     QVariantAnimation expandEnimation_;
     bool isExpanded_;
};

} // namespace PreferencesWindow

#endif // PACKETSIZEITEM_H
