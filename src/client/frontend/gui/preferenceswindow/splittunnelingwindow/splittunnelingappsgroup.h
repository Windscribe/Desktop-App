#pragma once

#include <atomic>
#include <memory>

#include <QThreadPool>

#include "commongraphics/baseitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/splittunneling.h"
#include "appincludeditem.h"
#include "appsearchitem.h"
#include "searchlineedititem.h"
#include "splittunnelingappsitem.h"

namespace PreferencesWindow {

struct EnumerationResult;

class SplitTunnelingAppsGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    static constexpr int kMaxApps = types::SplitTunneling::kMaxApps;

    explicit SplitTunnelingAppsGroup(ScalableGraphicsObject *parent,
                                     const QString &desc = "",
                                     const QString &descUrl = "");
    ~SplitTunnelingAppsGroup() override;

    QList<types::SplitTunnelingApp> apps();
    void setApps(QList<types::SplitTunnelingApp> apps);
    void addApp(types::SplitTunnelingApp &app);
    void ensureSearchAppsPopulated();

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
    void onAppsEnumerated(std::shared_ptr<EnumerationResult> result);
    void requestIcons(const QStringList &appPaths);
    bool addAppInternal(types::SplitTunnelingApp &app);
    void addSearchApp(types::SplitTunnelingApp &app);
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
    bool searchAppsRequested_;
    QThreadPool threadPool_;
    std::atomic<bool> stopRequested_;
};

} // namespace PreferencesWindow
