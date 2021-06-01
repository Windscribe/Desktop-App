#include "splittunnelingappssearchitem.h"

#include <QFileInfo>
#include "utils/utils.h"
#include "dpiscalemanager.h"
#ifdef Q_OS_WIN
    #include "utils/winutils.h"
#else
    #include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingAppsSearchItem::SplitTunnelingAppsSearchItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
    , loggedIn_(false)
{
    searchLineEditItem_ = new SearchLineEditItem(this);
    searchLineEditItem_->setPos(0, 0);
    connect(searchLineEditItem_, SIGNAL(textChanged(QString)), SLOT(onSearchTextChanged(QString)));
    connect(searchLineEditItem_, SIGNAL(searchModeExited()), SIGNAL(searchModeExited()));
    connect(searchLineEditItem_, SIGNAL(enterPressed()), SLOT(onSearchBoxEnterPressed()));
    connect(searchLineEditItem_, SIGNAL(focusIn()), SLOT(onSearchBoxFocusIn()));
}

void SplitTunnelingAppsSearchItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsSearchItem::getApps()
{
    return apps_;
}

void SplitTunnelingAppsSearchItem::setApps(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    apps_ = apps;

	updateProgramList();
}

void SplitTunnelingAppsSearchItem::setFocusOnSearchBar()
{
    searchLineEditItem_->setFocusOnSearchBar();
    searchLineEditItem_->setSelected(true);
}

void SplitTunnelingAppsSearchItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
}

void SplitTunnelingAppsSearchItem::keyReleaseEvent(QKeyEvent *event)
{
    ClickableGraphicsObject *selected = selectedObject();

    if (selected)
    {
        if (event->key() == Qt::Key_Up)
        {

            int selectedIndex = selectedObjectIndex();
            QList<ClickableGraphicsObject *> selectable = selectableObjects();
            if (selectedIndex > 0)
            {
                clearSelection();
                ClickableGraphicsObject * newlySelectedItem = selectable[selectedIndex-1];
                newlySelectedItem->setSelected(true);

                int posY = static_cast<int>(newlySelectedItem->pos().y());
                emit scrollToRect(QRect(0, posY, static_cast<int>(boundingRect().width()), 50));
            }
        }
        else if (event->key() == Qt::Key_Down)
        {
            int selectedIndex = selectedObjectIndex();

            QList<ClickableGraphicsObject *> selectable = selectableObjects();
            if (selectedIndex < selectable.length()-1)
            {
                clearSelection();
                ClickableGraphicsObject * newlySelectedItem = selectable[selectedIndex+1];
                newlySelectedItem->setSelected(true);

                int posY = static_cast<int>(newlySelectedItem->pos().y());
                emit scrollToRect(QRect(0, posY, static_cast<int>(boundingRect().width()), 50));
            }
        }
        else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            // see onSearchBoxEnterPressed function
        }
    }
    else // no selection
    {
        searchLineEditItem_->setSelected(true);
    }
}

void SplitTunnelingAppsSearchItem::onSearchTextChanged(QString text)
{
    clearSelection();
    updateDrawBasedOnFilterText(text);
    searchLineEditItem_->setSelected(true);

    if (text == "")
    {
        searchLineEditItem_->hideButtons();
    }
    else
    {
        searchLineEditItem_->showButtons();
    }
}

void SplitTunnelingAppsSearchItem::toggleAppItemActive(AppSearchItem *item)
{
    QString appName = item->getName();
    ProtoTypes::SplitTunnelingApp *existingApp = appByName(appName);

    if (existingApp) // USER or active system
    {
        if (existingApp->active())
        {
            existingApp->set_active(!existingApp->active());
            removeAppFromApps(*existingApp);
        }
    }
    else // inactive SYSTEM pressed
    {
        ProtoTypes::SplitTunnelingApp app;
        app.set_name(appName.toStdString());
        app.set_type(ProtoTypes::SPLIT_TUNNELING_APP_TYPE_SYSTEM);
        app.set_active(true);
        app.set_full_name(item->getFullName().toStdString());
        apps_.append(app);
    }

    updateDrawBasedOnFilterText(searchLineEditItem_->getText());
    emit appsUpdated(apps_);
}

void SplitTunnelingAppsSearchItem::updateProgramList()
{
    updateSystemApps();
    updateDrawBasedOnFilterText(searchLineEditItem_->getText());
}

void SplitTunnelingAppsSearchItem::updateScaling()
{
    BaseItem::updateScaling();
    updateItemsPosAndUpdateHeight();
}

bool SplitTunnelingAppsSearchItem::hasItemWithFocus()
{
    return searchLineEditItem_->hasItemWithFocus();
}

void SplitTunnelingAppsSearchItem::onAppItemClicked()
{
    if (loggedIn_)
    {
        AppSearchItem *item = static_cast<AppSearchItem*>(sender());

        if (item != nullptr)
        {
            toggleAppItemActive(item);
            setFocusOnSearchBar();
        }
    }
    else
    {
        emitNotLoggedInErrorMessage();
    }
}

void SplitTunnelingAppsSearchItem::onItemHoverLeave()
{
    clearSelection();
}

void SplitTunnelingAppsSearchItem::onItemHoverEnter()
{
    ClickableGraphicsObject *object = static_cast<ClickableGraphicsObject*>(sender());
    clearSelection();
    object->setSelected(true);
}

void SplitTunnelingAppsSearchItem::onSearchBoxEnterPressed()
{
    ClickableGraphicsObject *selected = selectedObject();
    if (selected != searchLineEditItem_)
    {
        int index = selectedObjectIndex();

        if (index != -1)
        {
            AppSearchItem *item = static_cast<AppSearchItem*>(selected);
            toggleAppItemActive(item);

            QList<ClickableGraphicsObject*> selectable = selectableObjects();
            if (index < selectable.length())
            {
                selectable[index]->setSelected(true);
            }
        }
    }
}

void SplitTunnelingAppsSearchItem::onSearchBoxFocusIn()
{
    clearSelection();
    searchLineEditItem_->setSelected(true);
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsSearchItem::filteredAppItems(QString filterString)
{
    QList<ProtoTypes::SplitTunnelingApp> apps;

    const auto appList = activeAndSystemApps();
    for (auto app : appList)
    {
        if (QString::fromStdString(app.name()).toLower().contains(filterString.toLower()))
        {
            apps.append(app);
        }
    }

    return apps;
}

ProtoTypes::SplitTunnelingApp *SplitTunnelingAppsSearchItem::appByName(QString appName)
{
    ProtoTypes::SplitTunnelingApp *found = nullptr;

    for (int i = 0; i < apps_.length(); i++)
    {
        ProtoTypes::SplitTunnelingApp *app = &apps_[i];

        if (app->name() == appName.toStdString())
        {
            found = app;
            break;
        }
    }

    return found;
}

void SplitTunnelingAppsSearchItem::updateDrawBasedOnFilterText(QString filterText)
{
    int drawPos = 50;
    if (filterText == "")
    {
        drawItemsAndUpdateHeight(drawPos, activeAndSystemApps());
    }
    else
    {
        drawItemsAndUpdateHeight(drawPos, filteredAppItems(filterText));
    }
}

void SplitTunnelingAppsSearchItem::drawItemsAndUpdateHeight(int baseHeight, QList<ProtoTypes::SplitTunnelingApp> itemsToDraw)
{
    drawnApps_.clear();

    int height = baseHeight*G_SCALE;
    for (auto app : qAsConst(itemsToDraw))
    {
        QString iconPath = QString::fromStdString(app.full_name());

#ifdef Q_OS_WIN
        iconPath = WinUtils::iconPathFromBinPath(iconPath);
#elif defined Q_OS_MAC
        iconPath = MacUtils::iconPathFromBinPath(iconPath);
#elif defined Q_OS_LINUX
        //todo linux
        Q_ASSERT(false);
#endif

        QSharedPointer<AppSearchItem> appItem = QSharedPointer<AppSearchItem>(new AppSearchItem(app, iconPath, this), &QObject::deleteLater);
        appItem->setClickable(true);
        connect(appItem.get(), SIGNAL(clicked()), SLOT(onAppItemClicked()));
        connect(appItem.get(), SIGNAL(hoverEnter()), SLOT(onItemHoverEnter()));
        connect(appItem.get(), SIGNAL(hoverLeave()), SLOT(onItemHoverLeave()));
        drawnApps_.append(appItem);

        appItem->setPos(0, height);
        height += 50*G_SCALE;
    }
    setHeight(height);
}

void SplitTunnelingAppsSearchItem::updateItemsPosAndUpdateHeight()
{
    int height = 50*G_SCALE;
    for (auto appItem : qAsConst(drawnApps_))
    {
        appItem->setPos(0, height);
        height += 50*G_SCALE;
    }

    setHeight(height);
}

void SplitTunnelingAppsSearchItem::removeAppFromApps(ProtoTypes::SplitTunnelingApp app)
{
    for (int i = 0; i < apps_.length(); i++)
    {
        if (apps_[i].name() == app.name())
        {
            apps_.removeAt(i);
        }
    }
}

void SplitTunnelingAppsSearchItem::updateSystemApps()
{
    systemApps_.clear();

#ifdef Q_OS_WIN
    const auto runningPrograms = WinUtils::enumerateRunningProgramLocations();
    for (const QString &exePath : runningPrograms)
    {
        if (!exePath.contains("C:\\Windows")
                && !exePath.contains("Windscribe.exe")
                && !exePath.contains("WindscribeEngine.exe"))
        {
            QFile f(exePath);
            QString name = QFileInfo(f).fileName();

            ProtoTypes::SplitTunnelingApp app;
            app.set_name(name.toStdString());
            app.set_type(ProtoTypes::SPLIT_TUNNELING_APP_TYPE_SYSTEM);
            app.set_full_name(exePath.toStdString());
            systemApps_.append(app);
        }
    }
#elif defined Q_OS_MAC
    const auto installedPrograms = MacUtils::enumerateInstalledPrograms();
    for (const QString &binPath : installedPrograms)
    {
        if (!binPath.contains("Windscribe"))
        {
            QFile f(binPath);
            QString name = QFileInfo(f).fileName();

            ProtoTypes::SplitTunnelingApp app;
            app.set_name(name.toStdString());
            app.set_type(ProtoTypes::SPLIT_TUNNELING_APP_TYPE_SYSTEM);
            app.set_full_name(binPath.toStdString());
            systemApps_.append(app);
        }
    }
#elif defined Q_OS_LINUX
        //todo linux
        Q_ASSERT(false);
#endif
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsSearchItem::activeAndSystemApps()
{
    QList<ProtoTypes::SplitTunnelingApp> activeAndSystemApps;

    const auto sortedApps = Utils::insertionSort(apps_);
    for (auto app : sortedApps)
    {
        activeAndSystemApps.append(app);
    }

    const auto sortedSystemApps = Utils::insertionSort(systemApps_);
    for (auto systemApp : sortedSystemApps)
    {
        bool found = false;
        for (const ProtoTypes::SplitTunnelingApp &app: qAsConst(activeAndSystemApps))
        {
            if (app.name() == systemApp.name())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            activeAndSystemApps.append(systemApp);
        }
    }

    return activeAndSystemApps;
}

void SplitTunnelingAppsSearchItem::emitNotLoggedInErrorMessage()
{
    QString title  = tr("Not Logged In");
    QString desc = tr("Please login to modify rules");
    emit nativeInfoErrorMessage(title, desc);
}

int SplitTunnelingAppsSearchItem::selectableIndex(ClickableGraphicsObject *object)
{
    int index = -1;

    QList<ClickableGraphicsObject *> selectable = selectableObjects();
    for (int i = 0; i < selectable.length(); i++)
    {
        if (selectable[i] == object)
        {
            index = i;
            break;
        }
    }

    return index;
}

int SplitTunnelingAppsSearchItem::selectedObjectIndex()
{
    int index = -1;

    QList<ClickableGraphicsObject *> selectable = selectableObjects();
    for (int i = 0; i < selectable.length(); i++)
    {
        if (selectable[i]->isSelected())
        {
            index = i;
            break;
        }
    }

    return index;
}

ClickableGraphicsObject *SplitTunnelingAppsSearchItem::selectedObject()
{
    ClickableGraphicsObject *selected = nullptr;

    const auto objectList = selectableObjects();
    for (auto *object : objectList)
    {
        if (object && object->isSelected())
        {
            selected = object;
            break;
        }
    }
    return selected;
}

QList<ClickableGraphicsObject *> SplitTunnelingAppsSearchItem::selectableObjects()
{
    QList<ClickableGraphicsObject*> selectable;

    selectable.append(static_cast<ClickableGraphicsObject *>(searchLineEditItem_));
    for (auto appItem : qAsConst(drawnApps_))
    {
        selectable.append(static_cast<ClickableGraphicsObject*>(appItem.get()));
    }

    return selectable;
}

void SplitTunnelingAppsSearchItem::clearSelection()
{
    const auto objectList = selectableObjects();
    for (auto *object : objectList)
    {
        if (object)
        {
            object->setSelected(false);
        }
    }
}


}
