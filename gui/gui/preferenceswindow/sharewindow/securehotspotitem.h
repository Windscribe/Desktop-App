#ifndef SECUREHOTSPOTITEM_H
#define SECUREHOTSPOTITEM_H

#include <QVariantAnimation>
#include "../baseitem.h"
#include "../editboxitem.h"
#include "../dividerline.h"
#include "../backend/preferences/preferences.h"
#include "commongraphics/checkboxbutton.h"
#include "../editboxitem.h"
#include <QVariantAnimation>

namespace PreferencesWindow {

class SecureHotspotItem : public BaseItem
{
    Q_OBJECT

public:
    enum HOTSPOT_SUPPORT_TYPE {HOTSPOT_SUPPORTED, HOTSPOT_NOT_SUPPORTED, HOTSPOT_NOT_SUPPORTED_BY_IKEV2 };

    explicit SecureHotspotItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setSecureHotspotPars(const ProtoTypes::ShareSecureHotspot &ss);
    void setSupported(HOTSPOT_SUPPORT_TYPE supported);

    void updateScaling() override;
    bool hasItemWithFocus() override;
signals:
    void secureHotspotParsChanged(const ProtoTypes::ShareSecureHotspot &ss);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onExpandAnimationValueChanged(const QVariant &value);
    void onSSIDChanged(const QString &text);
    void onPasswordChanged(const QString &password);

    void onLanguageChanged();

private:
    void updateCollapsedAndExpandedHeight();
    void updatePositions();

    int collapsedHeight_;
    int expandedHeight_;

    CheckBoxButton *checkBoxButton_;
    QVariantAnimation expandEnimation_;
    EditBoxItem *editBoxSSID_;
    EditBoxItem *editBoxPassword_;

    ProtoTypes::ShareSecureHotspot ss_;
    DividerLine *line_;

    QString descriptionText_;
    QRect descriptionRect_;

    HOTSPOT_SUPPORT_TYPE supported_;

};

} // namespace PreferencesWindow

#endif // SECUREHOTSPOTITEM_H
