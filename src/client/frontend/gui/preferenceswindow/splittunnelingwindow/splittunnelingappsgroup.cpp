#include "splittunnelingappsgroup.h"

#include <atomic>
#include <memory>
#include <type_traits>

#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include "preferenceswindow/preferencegroup.h"

#if defined(Q_OS_WIN)
    #include "utils/winutils.h"
    #include <shellapi.h>
    #include "widgetutils/widgetutils_win.h"
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#elif defined(Q_OS_MACOS)
    #include "utils/macutils.h"
#elif defined(Q_OS_LINUX)
    #include "utils/linuxutils.h"
#endif

namespace PreferencesWindow {

// What crosses the thread boundary. QPixmap cannot be built off the GUI thread, so the worker stops at a
// QImage -- or a raw icon handle on Windows -- and the GUI thread finishes the conversion.
struct IconPayload
{
    QImage image;
#if defined(Q_OS_WIN)
    // Owning and copyable, so the handle is released on every path -- including a result whose queued
    // delivery is dropped because the group was destroyed first.
    std::shared_ptr<std::remove_pointer_t<HICON>> handle;
#endif
};

struct EnumerationResult
{
    QList<types::SplitTunnelingApp> apps;
    QHash<QString, IconPayload> icons;      // keyed by SplitTunnelingApp::fullName
};

// Comfortably above APP_ICON_WIDTH at any scale factor and device pixel ratio we ship.
constexpr int kIconMaxPx = 128;

SplitTunnelingAppsGroup::SplitTunnelingAppsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), mode_(OP_MODE::DEFAULT), searchAppsRequested_(false),
    stopRequested_(false)
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
}

SplitTunnelingAppsGroup::~SplitTunnelingAppsGroup()
{
    stopRequested_ = true;
    threadPool_.waitForDone();
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
    setBatchMode(true);
    for (AppIncludedItem *item : apps_.keys()) {
        apps_.remove(item);
        hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    }

    QStringList paths;
    for (types::SplitTunnelingApp app : apps) {
        if (addAppInternal(app)) {
            paths << app.fullName;
        }
    }
    setBatchMode(false);
    // Before the first visit this runs at startup, and the first visit resolves these paths anyway.
    // Requesting here would put bundle reads back on the launch path for a screen that may never be opened.
    if (searchAppsRequested_) {
        requestIcons(paths);
    }
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::addApp(types::SplitTunnelingApp &app)
{
    if(addAppInternal(app)) {
        requestIcons({app.fullName});
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

namespace {

// Everything below the comment runs on the worker: these are the calls that block on a stalled volume.
QString resolveIconPath(const QString &appPath)
{
#if defined(Q_OS_MACOS)
    return MacUtils::iconPathFromBinPath(appPath);
#else
    return appPath;
#endif
}

IconPayload loadIconPayload(const QString &iconPath)
{
    IconPayload payload;
    if (iconPath.isEmpty()) {
        return payload;
    }
#if defined(Q_OS_WIN)
    // A read from a dead network share can stall indefinitely; a skipped icon just draws as the fallback.
    if (!WinUtils::isOnLocalVolume(iconPath)) {
        return payload;
    }
    if (iconPath.contains("WindowsApps")) {
        payload.image = WidgetUtils_win::extractWindowsAppProgramImage(iconPath);
    } else {
        HICON icon = ExtractIcon(NULL, iconPath.toStdWString().c_str(), 0);
        if (icon != NULL && (__int64)icon != 1) {
            payload.handle = std::shared_ptr<std::remove_pointer_t<HICON>>(icon, DestroyIcon);
        }
    }
#elif defined(Q_OS_MACOS)
    payload.image = QImage(iconPath);
#endif

    // A .icns or Store logo carries its largest representation -- 1024x1024 is common, 4 MB decoded -- and
    // these are held for the session. The row draws at APP_ICON_WIDTH, so cap well above that and no higher.
    if (payload.image.width() > kIconMaxPx || payload.image.height() > kIconMaxPx) {
        payload.image = payload.image.scaled(kIconMaxPx, kIconMaxPx, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return payload;
}

EnumerationResult enumerateInstalledApps(const std::atomic<bool> &stop)
{
    EnumerationResult result;
    QList<types::SplitTunnelingApp> apps;

#ifdef Q_OS_WIN
    const auto runningPrograms = WinUtils::enumerateRunningProgramLocations();
    for (const QString &exePath : runningPrograms) {
        if (exePath.contains("C:\\Windows") || exePath.contains("Windscribe.exe")) {
            continue;
        }
#elif defined Q_OS_MACOS
    const auto installedPrograms = MacUtils::enumerateInstalledPrograms();
    for (const QString &exePath : installedPrograms) {
        if (exePath.contains("Windscribe")) {
            continue;
        }
#elif defined Q_OS_LINUX
    const auto installedPrograms = LinuxUtils::enumerateInstalledPrograms();
    for (const QString &exePath : installedPrograms.keys()) {
        if (exePath.contains("Windscribe")) {
            continue;
        }
#endif
        if (stop) {
            return result;
        }
        QString name = QFileInfo(exePath).fileName();
#ifdef Q_OS_MACOS
        // On macOS, remove the ".app" suffix when used as a name
        if (name.endsWith(".app", Qt::CaseInsensitive)) {
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
        result.icons[app.fullName] = loadIconPayload(app.icon);
        apps << app;
    }

    result.apps = apps;
    return result;
}

QSharedPointer<IndependentPixmap> pixmapFromPayload(const IconPayload &payload)
{
    QPixmap pixmap;
#if defined(Q_OS_WIN)
    if (payload.handle) {
        pixmap = qt_pixmapFromWinHICON(payload.handle.get());
    }
#endif
    if (pixmap.isNull() && !payload.image.isNull()) {
        pixmap = QPixmap::fromImage(payload.image);
    }
    if (pixmap.isNull()) {
        return nullptr;
    }
    return QSharedPointer<IndependentPixmap>(new IndependentPixmap(pixmap));
}

}

void SplitTunnelingAppsGroup::ensureSearchAppsPopulated()
{
    if (searchAppsRequested_) {
        return;
    }
    searchAppsRequested_ = true;

    QStringList includedPaths;
    for (const types::SplitTunnelingApp &app : apps_.values()) {
        includedPaths << app.fullName;
    }

    // The saved apps are the only rows visible when the screen opens, so their icons ship ahead of the scan.
    requestIcons(includedPaths);

    // Reads and decodes every installed app bundle, so it runs off the GUI thread. Workers refuse to touch
    // non-local volumes, so a read cannot stall on a dead mount and the destructor's join is bounded by
    // one item's work on a local disk.
    threadPool_.start([this]() {
        std::shared_ptr<EnumerationResult> result =
            std::make_shared<EnumerationResult>(enumerateInstalledApps(stopRequested_));
        if (stopRequested_) {
            return;
        }
        QMetaObject::invokeMethod(this, [this, result]() { onAppsEnumerated(result); }, Qt::QueuedConnection);
    });
}

void SplitTunnelingAppsGroup::requestIcons(const QStringList &appPaths)
{
#if defined(Q_OS_LINUX)
    // Linux draws icons with QIcon::fromTheme in paint(), so there is no payload to fetch.
    Q_UNUSED(appPaths);
#else
    if (appPaths.isEmpty()) {
        return;
    }

    threadPool_.start([this, appPaths]() {
        std::shared_ptr<EnumerationResult> result = std::make_shared<EnumerationResult>();
        for (const QString &appPath : appPaths) {
            if (stopRequested_) {
                return;
            }
            result->icons[appPath] = loadIconPayload(resolveIconPath(appPath));
        }
        QMetaObject::invokeMethod(this, [this, result]() { onAppsEnumerated(result); }, Qt::QueuedConnection);
    });
#endif
}

void SplitTunnelingAppsGroup::onAppsEnumerated(std::shared_ptr<EnumerationResult> result)
{
    // Every add relays out the whole group, so batch the insert into a single layout pass.
    setBatchMode(true);
    for (types::SplitTunnelingApp &app : result->apps) {
        addSearchApp(app);
    }
    setBatchMode(false);

    // Converted once per path: an app can appear in both lists, and the payload is consumed on conversion.
    QHash<QString, QSharedPointer<IndependentPixmap>> pixmaps;
    for (auto it = result->icons.cbegin(); it != result->icons.cend(); ++it) {
        QSharedPointer<IndependentPixmap> pixmap = pixmapFromPayload(it.value());
        if (pixmap) {
            pixmaps[it.key()] = pixmap;
        }
    }

    for (auto it = searchApps_.cbegin(); it != searchApps_.cend(); ++it) {
        auto pixmap = pixmaps.constFind(it.value().fullName);
        if (pixmap != pixmaps.constEnd()) {
            it.key()->setIcon(pixmap.value());
        }
    }
    for (auto it = apps_.cbegin(); it != apps_.cend(); ++it) {
        auto pixmap = pixmaps.constFind(it.value().fullName);
        if (pixmap != pixmaps.constEnd()) {
            it.key()->setIcon(pixmap.value());
        }
    }

    if (mode_ == OP_MODE::SEARCH) {
        showFilteredSearchItems(searchLineEditItem_->text());
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
            // The row that was just clicked already holds the decoded icon, so hand it over rather than
            // reading it again: nothing else populates an included item created on this path.
            AppIncludedItem *addedApp = appByName(appName);
            if (addedApp) {
                addedApp->setIcon(item->icon());
            }
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
