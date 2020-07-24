#ifndef DNSWHILECONNECTEDITEM_H
#define DNSWHILECONNECTEDITEM_H

#include "../baseitem.h"
#include "../backend/preferences/preferences.h"
#include "../comboboxitem.h"
#include "../editboxitem.h"
#include "../dividerline.h"
#include "types/dnswhileconnectedtype.h"

#include <QVariantAnimation>

namespace PreferencesWindow {

class DNSWhileConnectedItem : public BaseItem
{
    Q_OBJECT
public:
    explicit DNSWhileConnectedItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    //void setDNSWhileConnected(const DNSWhileConnected &dns);

    void updateScaling() override;
signals:
    //void dnsWhileConnectedChanged(const DNSWhileConnected &dns);

private slots:
    void onDNSWhileConnectedModeChanged(QVariant v);
    void onDNSWhileConnectedIPChanged(QString v);

    void onExpandAnimationValueChanged(const QVariant &value);

    void onLanguageChanged() override;

protected:
    void hideOpenPopups() override;

private:
    const int COLLAPSED_HEIGHT = 50;
    const int EXPANDED_HEIGHT = 50 + 43;

    ComboBoxItem *comboBoxDNS_;
    EditBoxItem *editBoxIP_;

    QVariantAnimation expandEnimation_;
    bool isExpanded_;

    //DNSWhileConnected curDNSWhileConnected_;
    DividerLine *dividerLine_;

};

}
#endif // DNSWHILECONNECTEDITEM_H
