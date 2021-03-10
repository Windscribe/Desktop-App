#include "splittunnelingappsitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

#ifdef Q_OS_WIN
    #include "utils/winutils.h"
#else
    #include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingAppsItem::SplitTunnelingAppsItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
    , loggedIn_(false)
    , atLeastOneApp_(false)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    searchButton_ = new IconButton(16, 16, "preferences/VIEW_LOG_ICON", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    searchButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    searchButton_->setHoverOpacity(OPACITY_FULL);
    connect(searchButton_, SIGNAL(clicked()), SIGNAL(searchClicked()));
    connect(searchButton_, SIGNAL(selectionChanged(bool)), SLOT(onSelectionChanged(bool)));
    connect(searchButton_, SIGNAL(hoverEnter()), SLOT(onButtonHoverEnter()));

    addButton_ = new IconButton(16, 16, "locations/EXPAND_ICON", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    addButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    addButton_->setHoverOpacity(OPACITY_FULL);
    connect(addButton_, SIGNAL(clicked()), SLOT(onAddClicked()));
    connect(addButton_, SIGNAL(selectionChanged(bool)), SLOT(onSelectionChanged(bool)));
    connect(searchButton_, SIGNAL(hoverEnter()), SLOT(onButtonHoverEnter()));

    dividerLine_ = new PreferencesWindow::DividerLine(this, boundingRect().width());

    updatePositions();
}

SplitTunnelingAppsItem::~SplitTunnelingAppsItem()
{
    delete dividerLine_;
}

void SplitTunnelingAppsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // desc
    qreal initOpacity = painter->opacity();
    painter->setOpacity(OPACITY_UNHOVER_TEXT * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    QString textDesc = tr("Select the apps you wish to include for split tunneling below");
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 16*G_SCALE, -32*G_SCALE, 0), textDesc);

    // header
    painter->setFont(*FontManager::instance().getFont(14, true));
    QString textHeader = tr("Search/Add Apps");
    painter->drawText(QRect(16*G_SCALE, 60*G_SCALE, boundingRect().width(), 50*G_SCALE), textHeader);

    // No apps text
    if (!atLeastOneApp_)
    {
        painter->setFont(*FontManager::instance().getFont(14, false));
        painter->drawText(boundingRect().adjusted(0, 175*G_SCALE, 0, 175*G_SCALE), Qt::AlignHCenter, tr("No Apps Added"));
    }
}

QList<ProtoTypes::SplitTunnelingApp> SplitTunnelingAppsItem::getApps()
{
    return apps_;
}

void SplitTunnelingAppsItem::setApps(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    apps_ = apps;
    drawItemsAndUpdateHeight();
}

void SplitTunnelingAppsItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
}

void SplitTunnelingAppsItem::keyReleaseEvent(QKeyEvent *event)
{
    ClickableGraphicsObject *selected = selectedObject();

    if (event->key() == Qt::Key_Escape)
    {
        if (selected)
        {
            selected->setSelected(false);
        }
        else
        {
            emit escape();
        }
    }
    else if (selected)
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
            if (selected == searchButton_)
            {
                emit searchClicked();
            }
            else if (selected == addButton_)
            {
                attemptAdd();
            }
            else
            {
                attemptDelete(static_cast<AppIncludedItem *>(selected));
            }
        }
    }
    else // no selection
    {
        searchButton_->setSelected(true);
    }
}

void SplitTunnelingAppsItem::attemptDelete(AppIncludedItem *appItem)
{
    if (loggedIn_)
    {
        if (appItem != nullptr)
        {
            int index = selectableIndex(appItem);

            ProtoTypes::SplitTunnelingApp *app = appByName(appItem->getName());
            app->set_active(false);
            disconnect(appItem);
            removeAppFromApps(*app);
            drawItemsAndUpdateHeight();

            // select closest item for new selection
            const QList<ClickableGraphicsObject*> &selectable = selectableObjects();
            if (index >= selectable.length()) index = selectable.length() - 1;
            if (index >= 0)
            {
                selectable[index]->setSelected(index);
            }

            emit appsUpdated(apps_);
        }
    }
    else
    {
        emitNotLoggedInErrorMessage();
    }
}

void SplitTunnelingAppsItem::onAppDeleteClicked()
{
    AppIncludedItem *appItem(static_cast<AppIncludedItem*>(sender()));

    attemptDelete(appItem);
}

void SplitTunnelingAppsItem::attemptAdd()
{
    if (loggedIn_)
    {
        emit addClicked();
    }
    else
    {
        emitNotLoggedInErrorMessage();
    }
}

void SplitTunnelingAppsItem::updateScaling()
{
    BaseItem::updateScaling();
    updatePositions();
    updateItemsPosAndUpdateHeight();
}

void SplitTunnelingAppsItem::onAddClicked()
{
    attemptAdd();
}

void SplitTunnelingAppsItem::onSelectionChanged(bool selected)
{
    if (selected)
    {
        ClickableGraphicsObject *newSelection = static_cast<ClickableGraphicsObject*>(sender());
        clearSelection();
        newSelection->setSelected(true);
    }
}

void SplitTunnelingAppsItem::onButtonHoverEnter()
{
    ClickableGraphicsObject *object = static_cast<ClickableGraphicsObject*>(sender());
    clearSelection();
    object->setSelected(true);
}

void SplitTunnelingAppsItem::onItemHoverLeave()
{
    clearSelection();
}

void SplitTunnelingAppsItem::drawItemsAndUpdateHeight()
{
    drawnApps_.clear();

    int height = 90*G_SCALE;
    const auto sortedApps = Utils::insertionSort(apps_);
    for (auto app : sortedApps)
    {
        QString iconPath = QString::fromStdString(app.full_name());

#ifdef Q_OS_MAC
        iconPath = MacUtils::iconPathFromBinPath(iconPath);
#else
        iconPath = WinUtils::iconPathFromBinPath(iconPath);
#endif

        QSharedPointer<AppIncludedItem> appItem = QSharedPointer<AppIncludedItem>(new AppIncludedItem(app, iconPath, this), &QObject::deleteLater);
        connect(appItem.get(), SIGNAL(deleteClicked()), SLOT(onAppDeleteClicked()));
        connect(appItem.get(), SIGNAL(selectionChanged(bool)), SLOT(onSelectionChanged(bool)));
        connect(appItem.get(), SIGNAL(hoverLeave()), SLOT(onItemHoverLeave()));

        drawnApps_.append(appItem);

        appItem->setPos(0, height);
        height += 50*G_SCALE;
    }

    atLeastOneApp_ = drawnApps_.length() > 0;

    if (height < 200*G_SCALE) height = 200*G_SCALE; // for "No Apps Added" Text visibility
    setHeight(height);
    update();
}

void SplitTunnelingAppsItem::updateItemsPosAndUpdateHeight()
{
    int height = 90*G_SCALE;
    for (auto appItem : qAsConst(drawnApps_))
    {
        appItem->setPos(0, height);
        height += 50*G_SCALE;
    }

    if (height < 200*G_SCALE) height = 200*G_SCALE; // for "No Apps Added" Text visibility
    setHeight(height);
}

ProtoTypes::SplitTunnelingApp *SplitTunnelingAppsItem::appByName(QString appName)
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

void SplitTunnelingAppsItem::removeAppFromApps(ProtoTypes::SplitTunnelingApp app)
{
    for (int i = 0; i < apps_.length(); i++)
    {
        if (apps_[i].name() == app.name())
        {
            apps_.removeAt(i);
        }
    }
}

void SplitTunnelingAppsItem::emitNotLoggedInErrorMessage()
{
    QString title = tr("Not Logged In");
    QString desc  = tr("Please login to modify rules");
    emit nativeInfoErrorMessage(title, desc);
}

int SplitTunnelingAppsItem::selectableIndex(ClickableGraphicsObject *object)
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

int SplitTunnelingAppsItem::selectedObjectIndex()
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

ClickableGraphicsObject *SplitTunnelingAppsItem::selectedObject()
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

QList<ClickableGraphicsObject *> SplitTunnelingAppsItem::selectableObjects()
{
    QList<ClickableGraphicsObject*> selectable;

    selectable.append(static_cast<ClickableGraphicsObject *>(searchButton_));
    selectable.append(static_cast<ClickableGraphicsObject *>(addButton_));
    for (auto appItem : qAsConst(drawnApps_))
    {
        selectable.append(static_cast<ClickableGraphicsObject*>(appItem.get()));
    }

    return selectable;
}

void SplitTunnelingAppsItem::clearSelection()
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

void SplitTunnelingAppsItem::updatePositions()
{
    searchButton_->setPos(boundingRect().width() - 64*G_SCALE, 60*G_SCALE);
    addButton_->setPos(boundingRect().width() - 32*G_SCALE, 60*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, 85*G_SCALE);

}

}
