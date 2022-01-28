#ifndef PACKETSIZEITEM_H
#define PACKETSIZEITEM_H

#include <QTimer>
#include "../baseitem.h"
#include "backend/preferences/preferences.h"
#include "automanualswitchitem.h"

#include <QVariantAnimation>
#include "packetsizeeditboxitem.h"
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
    void showPacketSizeDetectionError(const QString &title, const QString &message);

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void packetSizeChanged(const ProtoTypes::PacketSize &ps);
    void detectAppropriatePacketSizeButtonClicked();

private slots:
    void onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state);
    void onEditBoxTextChanged(const QString &text);
    void onExpandAnimationValueChanged(const QVariant &value);

    void onAutoDetectAndGenerateHoverEnter();
    void onAutoDetectAndGenerateHoverLeave();

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43;

    AutoManualSwitchItem *switchItem_;
    PacketSizeEditBoxItem *editBoxPacketSize_;

    ProtoTypes::PacketSize curPacketSize_;

     QVariantAnimation expandEnimation_;
     bool isExpanded_;
     bool isShowingError_;
};

} // namespace PreferencesWindow

#endif // PACKETSIZEITEM_H
