#ifndef SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H
#define SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H

#include "../basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "splittunnelingappssearchitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsSearchWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsSearchWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    void setFocusOnSearchBar();

    QList<types::SplitTunnelingApp> getApps();
    void setApps(QList<types::SplitTunnelingApp> apps);
    void setLoggedIn(bool loggedIn);

    void updateProgramList();

signals:
    void appsUpdated(QList<types::SplitTunnelingApp>);
    void searchModeExited();
    void nativeInfoErrorMessage(QString,QString);

private slots:
    void onAppsUpdated(QList<types::SplitTunnelingApp> apps);

private:
    SplitTunnelingAppsSearchItem *splitTunnelingAppsSearchItem_;
    Preferences *preferences_;

};

} // namespace

#endif // SPLITTUNNELINGAPPSSEARCHWINDOWITEM_H
