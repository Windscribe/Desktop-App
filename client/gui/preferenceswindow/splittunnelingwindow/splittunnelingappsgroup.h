#pragma once

#include "commongraphics/baseitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "appincludeditem.h"
#include "appsearchitem.h"
#include "searchlineedititem.h"
#include "splittunnelingappsitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    static const int kMaxApps = 50;

    explicit SplitTunnelingAppsGroup(ScalableGraphicsObject *parent,
                                     const QString &desc = "",
                                     const QString &descUrl = "");

    QList<types::SplitTunnelingApp> apps();
    void setApps(QList<types::SplitTunnelingApp> apps);
    void addApp(types::SplitTunnelingApp &app);

    void setLoggedIn(bool loggedIn);

signals:
    void searchClicked();
    void addClicked();
    void appsUpdated(QList<types::SplitTunnelingApp> apps);
    void escape();
    void setError(QString msg);

protected slots:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onSearchClicked();
    void onDeleteClicked();
    void onSearchTextChanged(QString text);
    void onSearchModeExited();
    void onSearchBoxFocusIn();
    void onSearchItemClicked();

private:
    bool addAppInternal(types::SplitTunnelingApp &app);
    void addSearchApp(types::SplitTunnelingApp &app);
    void populateSearchApps();
    void showFilteredSearchItems(QString filter);
    void toggleAppItemActive(AppSearchItem *item);
    AppIncludedItem *appByName(QString name);

    enum OP_MODE {
        DEFAULT = 0,
        SEARCH = 1,
    };
    SplitTunnelingAppsItem *splitTunnelingAppsItem_;
    SearchLineEditItem *searchLineEditItem_;

    QMap<AppIncludedItem *, types::SplitTunnelingApp> apps_;
    QMap<AppSearchItem *, types::SplitTunnelingApp> searchApps_;

    OP_MODE mode_;
};

} // namespace PreferencesWindow
