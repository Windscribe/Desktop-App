#ifndef SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H
#define SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H

#include "../basepage.h"
#include "../Backend/Preferences/preferences.h"
#include "../Backend/Preferences/preferenceshelper.h"
#include "splittunnelingappssearchitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsSearchWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsSearchWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    void setFocusOnSearchBar();

    QList<ProtoTypes::SplitTunnelingApp> getApps();
    void setApps(QList<ProtoTypes::SplitTunnelingApp> apps);
    void setLoggedIn(bool loggedIn);

    void updateProgramList();

signals:
    void appsUpdated(QList<ProtoTypes::SplitTunnelingApp>);
    void searchModeExited();
    void nativeInfoErrorMessage(QString,QString);

private slots:
    void onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);

private:
    SplitTunnelingAppsSearchItem *splitTunnelingAppsSearchItem_;
    Preferences *preferences_;

};

} // namespace

#endif // SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H
