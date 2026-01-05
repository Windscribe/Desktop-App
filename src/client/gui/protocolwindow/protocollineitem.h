#pragma once

#include "../../common/types/protocolstatus.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "graphicresources/independentpixmap.h"

namespace ProtocolWindow {

class ProtocolLineItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ProtocolLineItem(ScalableGraphicsObject *parent, const types::ProtocolStatus &status, const QString &desc);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    types::Protocol protocol();
    uint port();
    types::ProtocolStatus::Status status();
    int decrementCountdown();
    void clearCountdown();

    void setDescription(const QString &desc);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private slots:
    void onHoverValueChanged(const QVariant &value);

private:
    static constexpr int kRoundedRectRadius = 8;
    static constexpr int kBorderWidth = 2;
    static constexpr int kTextIndent = 16;
    static constexpr int kBannerY = 4;
    static constexpr int kFirstLineY = 13;
    static constexpr int kSecondLineY = 35;
    static constexpr int kDividerTop = 18;
    static constexpr int kDividerBottom = 28;

    types::ProtocolStatus status_;
    QString desc_;
    IconButton *icon_; // > arrow

    QVariantAnimation hoverAnimation_;
    double hoverValue_;
};

} // namespace ProtocolWindow