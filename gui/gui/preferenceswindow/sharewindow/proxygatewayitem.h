#ifndef PROXYGATEWAYITEM_H
#define PROXYGATEWAYITEM_H

#include "../baseitem.h"
#include "../dividerline.h"
#include "../comboboxitem.h"
#include "CommonGraphics/checkboxbutton.h"
#include "proxyipaddressitem.h"
#include "../Backend/Preferences/preferences.h"
#include "../Backend/Preferences/preferenceshelper.h"
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {

class ProxyGatewayItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ProxyGatewayItem(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setProxyGatewayPars(const ProtoTypes::ShareProxyGateway &sp);

    void updateScaling() override;

signals:
    void proxyGatewayParsChanged(const ProtoTypes::ShareProxyGateway &sp);
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onExpandAnimationValueChanged(const QVariant &value);
    void onProxyTypeItemChanged(QVariant v);

    void onProxyGatewayAddressChanged(const QString &address);

    void onLanguageChanged() override;

protected:
    void hideOpenPopups() override;

private:    
    void updateCollapsedAndExpandedHeights();
    void updatePositions();

    int collapsedHeight_;
    int expandedHeight_;

    CheckBoxButton *checkBoxButton_;
    QVariantAnimation expandEnimation_;
    ComboBoxItem *comboBoxProxyType_;
    ProxyIpAddressItem *proxyIpAddressItem_;

    ProtoTypes::ShareProxyGateway sp_;
    DividerLine *line_;

    QString descriptionText_;
    QRect descriptionRect_;
    //QFont descriptionFont_;
};

} // namespace PreferencesWindow

#endif // PROXYGATEWAYITEM_H
