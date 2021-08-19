#ifndef SPLITTUNNELINGAPPWINDOWITEM_H
#define SPLITTUNNELINGAPPWINDOWITEM_H

#include "preferenceswindow/basepage.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "splittunnelingappsitem.h"
#include "splittunnelingappssearchitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    QList<ProtoTypes::SplitTunnelingApp> getApps();
    void setApps(QList<ProtoTypes::SplitTunnelingApp> apps);
    void addAppManually(ProtoTypes::SplitTunnelingApp app);
    void setLoggedIn(bool able);

signals:
    void appsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);
    void searchButtonClicked();
    void addButtonClicked();
    void nativeInfoErrorMessage(QString title, QString desc);

private slots:
    void onAppsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);

private:
    Preferences *preferences_;
    SplitTunnelingAppsItem *splitTunnelingAppsItem_;

};

}

#endif // SPLITTUNNELINGAPPWINDOWITEM_H
