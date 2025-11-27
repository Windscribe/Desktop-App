#include "splittunnelingappsgroup.h"

#include <QFileInfo>
#include <QPainter>
#include "preferenceswindow/preferencegroup.h"

#if defined(Q_OS_WIN)
    #include "utils/winutils.h"
#elif defined(Q_OS_MACOS)
    #include "utils/macutils.h"
#elif defined(Q_OS_LINUX)
    #include "utils/linuxutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingAppsGroup::SplitTunnelingAppsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), mode_(OP_MODE::DEFAULT)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    splitTunnelingAppsItem_ = new SplitTunnelingAppsItem(this);
    connect(splitTunnelingAppsItem_, &SplitTunnelingAppsItem::addClicked, this, &SplitTunnelingAppsGroup::addClicked);
    connect(splitTunnelingAppsItem_, &SplitTunnelingAppsItem::searchClicked, this, &SplitTunnelingAppsGroup::onSearchClicked);
    addItem(splitTunnelingAppsItem_);

    searchLineEditItem_ = new SearchLineEditItem(this);
    connect(searchLineEditItem_, &SearchLineEditItem::textChanged, this, &SplitTunnelingAppsGroup::onSearchTextChanged);
    connect(searchLineEditItem_, &SearchLineEditItem::searchModeExited, this, &SplitTunnelingAppsGroup::onSearchModeExited);
    connect(searchLineEditItem_, &SearchLineEditItem::focusIn, this, &SplitTunnelingAppsGroup::onSearchBoxFocusIn);
    addItem(searchLineEditItem_);

    hideItems(indexOf(searchLineEditItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    populateSearchApps();
}

QList<types::SplitTunnelingApp> SplitTunnelingAppsGroup::apps()
{
    return apps_.values();
}

void SplitTunnelingAppsGroup::setApps(QList<types::SplitTunnelingApp> apps)
{
    if (apps == apps_.values()) {
        return;
    }
    for (AppIncludedItem *item : apps_.keys()) {
        apps_.remove(item);
        hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    }

    for (types::SplitTunnelingApp app : apps) {
        addAppInternal(app);
    }
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::addApp(types::SplitTunnelingApp &app)
{
    if(addAppInternal(app)) {
        emit appsUpdated(apps_.values());
    }
}

bool SplitTunnelingAppsGroup::addAppInternal(types::SplitTunnelingApp &app)
{
    if (apps_.size() >= kMaxApps) {
        emit setError(tr("There are too many apps in the list. Please remove some before adding more."));
        return false;
    }

    AppIncludedItem *item = new AppIncludedItem(app, this);
    connect(item, &AppIncludedItem::deleteClicked, this, &SplitTunnelingAppsGroup::onDeleteClicked);
    connect(item, &AppIncludedItem::activeChanged, this, [this, item](bool active) {
        apps_[item].active = active;
        emit appsUpdated(apps_.values());
    });
    apps_[item] = app;

    addItem(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    if (mode_ == OP_MODE::DEFAULT) {
        showItems(indexOf(item));
    }
    return true;
}

void SplitTunnelingAppsGroup::addSearchApp(types::SplitTunnelingApp &app)
{
    AppSearchItem *item = new AppSearchItem(app, this);
    item->setClickable(true);
    connect(item, &AppSearchItem::clicked, this, &SplitTunnelingAppsGroup::onSearchItemClicked);
    searchApps_[item] = app;

    addItem(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    if (mode_ == OP_MODE::SEARCH) {
        showItems(indexOf(item));
    }
}

void SplitTunnelingAppsGroup::onSearchClicked()
{
    mode_ = OP_MODE::SEARCH;

    // hide everything
    hideItems(0, size() - 1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    // show search bar and search items
    showItems(indexOf(searchLineEditItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    showFilteredSearchItems(searchLineEditItem_->text());

    searchLineEditItem_->setFocusOnSearchBar();
}

void SplitTunnelingAppsGroup::onDeleteClicked()
{
    AppIncludedItem *item = static_cast<AppIncludedItem *>(sender());
    apps_.remove(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::onSearchTextChanged(QString text)
{
    showFilteredSearchItems(text);
    searchLineEditItem_->setSelected(true);

    if (text == "") {
        searchLineEditItem_->hideButtons();
    } else {
        searchLineEditItem_->showButtons();
    }
}

void SplitTunnelingAppsGroup::onSearchModeExited()
{
    mode_ = OP_MODE::DEFAULT;

    searchLineEditItem_->setText("");

    // hide everything
    hideItems(0, size() - 1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    // show main bar and active items
    showItems(indexOf(splitTunnelingAppsItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    for (BaseItem *item : apps_.keys()) {
        showItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    }

    // take focus away from the search bar
    setFocus(Qt::PopupFocusReason);
}

void SplitTunnelingAppsGroup::onSearchBoxFocusIn()
{
    searchLineEditItem_->setSelected(true);
}

void SplitTunnelingAppsGroup::populateSearchApps()
{
    searchApps_.clear();

#ifdef Q_OS_WIN
    const auto runningPrograms = WinUtils::enumerateRunningProgramLocations();
    for (const QString &exePath : runningPrograms) {
        if (!exePath.contains("C:\\Windows") && !exePath.contains("Windscribe.exe")) {
#elif defined Q_OS_MACOS
    const auto installedPrograms = MacUtils::enumerateInstalledPrograms();
    for (const QString &exePath : installedPrograms) {
        if (!exePath.contains("Windscribe")) {
#elif defined Q_OS_LINUX
    const auto installedPrograms = LinuxUtils::enumerateInstalledPrograms();
    for (const QString &exePath : installedPrograms.keys()) {
        if (!exePath.contains("Windscribe")) {
#endif
            QFile f(exePath);
            QString name = QFileInfo(f).fileName();
#ifdef Q_OS_MACOS
            // On macOS, remove the ".app" suffix when used as a name
            if (name.endsWith(".app")) {
                name = name.left(name.length() - 4);
            }
#endif
            types::SplitTunnelingApp app;
            app.name = name;
            app.type = SPLIT_TUNNELING_APP_TYPE_SYSTEM;
            app.fullName = exePath;
#ifdef Q_OS_WIN
            app.icon = app.fullName;
#elif defined Q_OS_MACOS
            app.icon = MacUtils::iconPathFromBinPath(exePath);
#elif defined Q_OS_LINUX
            app.icon = installedPrograms[exePath];
#endif
            addSearchApp(app);
        }
    }
}

void SplitTunnelingAppsGroup::onSearchItemClicked()
{
    AppSearchItem *item = static_cast<AppSearchItem*>(sender());

    if (item != nullptr) {
        onSearchModeExited();
        toggleAppItemActive(item);
    }
}

void SplitTunnelingAppsGroup::toggleAppItemActive(AppSearchItem *item)
{
    QString appName = item->getName();
    AppIncludedItem *existingApp = appByName(appName);

    if (!existingApp) {
        types::SplitTunnelingApp app;
        app.name = appName;
        app.type = SPLIT_TUNNELING_APP_TYPE_SYSTEM;
        app.active = true;
        app.fullName = item->getFullName();
        app.icon = item->getAppIcon();
        if (addAppInternal(app)) {
            emit appsUpdated(apps_.values());
        }
    }
}

AppIncludedItem *SplitTunnelingAppsGroup::appByName(QString name)
{
    for (AppIncludedItem *item : apps_.keys()) {
        if (apps_[item].name == name) {
            return item;
        }
    }
    return nullptr;
}

void SplitTunnelingAppsGroup::showFilteredSearchItems(QString filter)
{
    if (OP_MODE::SEARCH) {
        for (AppSearchItem *item : searchApps_.keys()) {
            if (filter == "" || (searchApps_[item].name.toLower().contains(filter.toLower()) && !appByName(searchApps_[item].name))) {
                showItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
            } else {
                hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
            }
        }
    }
}

void SplitTunnelingAppsGroup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (mode_ == OP_MODE::SEARCH) {
            if (searchLineEditItem_->text() == "") {
                onSearchModeExited();
            } else {
                searchLineEditItem_->setText("");
                emit escape();
            }
        } else {
            emit escape();
        }
    }
}

void SplitTunnelingAppsGroup::setLoggedIn(bool loggedIn)
{
    splitTunnelingAppsItem_->setClickable(loggedIn);
    searchLineEditItem_->setClickable(loggedIn);
    if (mode_ == OP_MODE::SEARCH) {
        onSearchModeExited();
    }

    for (AppIncludedItem *item : apps_.keys()) {
        item->setClickable(loggedIn);
    }
}

} // namespace PreferencesWindow
