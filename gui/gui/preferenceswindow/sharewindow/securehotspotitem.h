#ifndef SECUREHOTSPOTITEM_H
#define SECUREHOTSPOTITEM_H

#include <QVariantAnimation>
#include "../baseitem.h"
#include "../editboxitem.h"
#include "../dividerline.h"
#include "../Backend/Preferences/preferences.h"
#include "CommonGraphics/checkboxbutton.h"
#include "../editboxitem.h"
#include <QVariantAnimation>
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {

class SecureHotspotItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SecureHotspotItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void setSecureHotspotPars(const ProtoTypes::ShareSecureHotspot &ss);
    void setSupported(bool bSupported);

    void updateScaling() override;

signals:
    void secureHotspotParsChanged(const ProtoTypes::ShareSecureHotspot &ss);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onExpandAnimationValueChanged(const QVariant &value);
    void onSSIDChanged(const QString &text);
    void onPasswordChanged(const QString &password);

    void onLanguageChanged() override;

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

    bool bSupported_;

};

} // namespace PreferencesWindow

#endif // SECUREHOTSPOTITEM_H
