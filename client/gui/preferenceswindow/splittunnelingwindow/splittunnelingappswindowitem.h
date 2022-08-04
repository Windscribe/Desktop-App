#ifndef SPLITTUNNELINGAPPWINDOWITEM_H
#define SPLITTUNNELINGAPPWINDOWITEM_H

#include "preferenceswindow/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "splittunnelingappsitem.h"
#include "splittunnelingappssearchitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    QList<types::SplitTunnelingApp> getApps();
    void setApps(QList<types::SplitTunnelingApp> apps);
    void addAppManually(types::SplitTunnelingApp app);
    void setLoggedIn(bool able);

signals:
    void appsUpdated(QList<types::SplitTunnelingApp> apps);
    void searchButtonClicked();
    void addButtonClicked();
    void nativeInfoErrorMessage(QString title, QString desc);

private slots:
    void onAppsUpdated(QList<types::SplitTunnelingApp> apps);

private:
    Preferences *preferences_;
    SplitTunnelingAppsItem *splitTunnelingAppsItem_;

};

}

#endif // SPLITTUNNELINGAPPWINDOWITEM_H
