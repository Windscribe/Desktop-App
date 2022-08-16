#ifndef SPLITTUNNELINGAPPSWINDOWITEM_H
#define SPLITTUNNELINGAPPSWINDOWITEM_H

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "splittunnelingappsgroup.h"

namespace PreferencesWindow {

class SplitTunnelingAppsWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    QList<types::SplitTunnelingApp> getApps();
    void setApps(QList<types::SplitTunnelingApp> apps);
    void addAppManually(types::SplitTunnelingApp app);

    void setLoggedIn(bool loggedIn);

signals:
    void appsUpdated(QList<types::SplitTunnelingApp> apps);
    void searchButtonClicked();
    void addButtonClicked();
    void escape();

private slots:
    void onAppsUpdated(QList<types::SplitTunnelingApp> apps);

private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    SplitTunnelingAppsGroup *splitTunnelingAppsGroup_;
};

}

#endif // SPLITTUNNELINGAPPSWINDOWITEM_H
