#ifndef PACKETSIZEGROUP_H
#define PACKETSIZEGROUP_H

#include <QGraphicsObject>
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "packetsizeeditboxitem.h"

namespace PreferencesWindow {

class PacketSizeGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit PacketSizeGroup(ScalableGraphicsObject *parent,
                             const QString &desc = "",
                             const QString &descUrl = "");

    void setPacketSizeSettings(types::PacketSize settings);
    void setPacketSizeDetectionState(bool on);
    void showPacketSizeDetectionError(const QString &title, const QString &message);

signals:
    void packetSizeChanged(const types::PacketSize &settings);
    void detectPacketSize();

private slots:
    void onAutomaticChanged(QVariant value);
    void onEditBoxTextChanged(const QString &text);
    void onDetectHoverEnter();
    void onDetectHoverLeave();

private:
    void updateMode();
    ComboBoxItem *packetSizeModeItem_;
    PacketSizeEditBoxItem *editBoxItem_;
    bool isShowingError_;

    types::PacketSize settings_;
};

} // namespace PreferencesWindow

#endif // PACKETSIZEGROUP_H
