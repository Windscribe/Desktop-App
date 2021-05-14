#ifndef DNSWHILECONNECTEDITEM_H
#define DNSWHILECONNECTEDITEM_H

#include "../baseitem.h"
#include "../backend/preferences/preferences.h"
#include "../comboboxitem.h"
#include "../editboxitem.h"
#include "../dividerline.h"
#include "../backend/types/dnswhileconnectedinfo.h"

#include <QVariantAnimation>

namespace PreferencesWindow {

class DnsWhileConnectedItem : public BaseItem
{
    Q_OBJECT
public:
    explicit DnsWhileConnectedItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setDNSWhileConnected(const DnsWhileConnectedInfo &dns);

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void dnsWhileConnectedInfoChanged(const DnsWhileConnectedInfo &dns);

private slots:
    void onDNSWhileConnectedModeChanged(QVariant v);
    void onDNSWhileConnectedIPChanged(QString v);

    void onExpandAnimationValueChanged(const QVariant &value);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43;

    ComboBoxItem *comboBoxDNS_;
    EditBoxItem *editBoxIP_;

    QVariantAnimation expandEnimation_;
    bool isExpanded_;

    DnsWhileConnectedInfo curDNSWhileConnected_;
    DividerLine *dividerLine_;

    void updateHeight(DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE type);
    void setHeightAndLinePos(int height);
};

}
#endif // DNSWHILECONNECTEDITEM_H
