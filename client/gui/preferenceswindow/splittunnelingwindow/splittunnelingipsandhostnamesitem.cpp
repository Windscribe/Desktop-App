#include "splittunnelingipsandhostnamesitem.h"

#include <QPainter>
#include <QMessageBox>
#include "graphicresources/fontmanager.h"
#include "utils/ipvalidation.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SplitTunnelingIpsAndHostnamesItem::SplitTunnelingIpsAndHostnamesItem(ScalableGraphicsObject *parent) : BaseItem (parent, 100)
    , loggedIn_(false)
{
    setFlags(flags() | QGraphicsItem::ItemIsFocusable);

    newIpOrHostnameItem_ = new NewIpOrHostnameItem(this);
    newIpOrHostnameItem_->setPos(0, 50);
    connect(newIpOrHostnameItem_, SIGNAL(addNewIpOrHostnameClicked(QString)), SLOT(onAddNewIpOrHostnameClicked(QString)));
    connect(newIpOrHostnameItem_, SIGNAL(escape()), SIGNAL(escape()));
}

void SplitTunnelingIpsAndHostnamesItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();
    painter->setOpacity(OPACITY_UNHOVER_TEXT * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    QString text = tr("Enter IP or hostname below that you wish to include for split tunneling");
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 16*G_SCALE, -32*G_SCALE, 0), text);
}

void SplitTunnelingIpsAndHostnamesItem::setNetworkRoutes(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes)
{
    for (auto route : qAsConst(routes))
    {
        createNetworkRouteUiItem(route);
    }
}

void SplitTunnelingIpsAndHostnamesItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
}

void SplitTunnelingIpsAndHostnamesItem::setFocusOnTextEntry()
{
    newIpOrHostnameItem_->setSelected(true);
}

void SplitTunnelingIpsAndHostnamesItem::keyReleaseEvent(QKeyEvent *event)
{
    ClickableGraphicsObject *selected = selectedObject();

    if (event->key() == Qt::Key_Escape)
    {
        if (selected)
        {
            selected->setSelected(false);
            setFocus();
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

            if (selectedIndex == 0)
            {
                setFocus();
            }

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
            IpOrHostnameItem * itemToDelete = static_cast<IpOrHostnameItem *>(selected);
            attemptDelete(itemToDelete);
        }
    }
    else
    {
        newIpOrHostnameItem_->setSelected(true);
    }
}

void SplitTunnelingIpsAndHostnamesItem::focusInEvent(QFocusEvent * /*event*/)
{
    ClickableGraphicsObject *selected = selectedObject();
    if (selected)
    {
        selected->setSelected(false);
    }
}

void SplitTunnelingIpsAndHostnamesItem::createNetworkRouteUiItem(ProtoTypes::SplitTunnelingNetworkRoute route)
{
    IpOrHostnameItem *item = new IpOrHostnameItem(route, this);
    ipsAndHostnames_.append(item);
    connect(item, SIGNAL(deleteClicked()), SLOT(onDeleteClicked()));
    connect(item, SIGNAL(selectionChanged(bool)), SLOT(onSelectionChanged(bool)));
    connect(item, SIGNAL(hoverLeave()), SLOT(onItemHoverLeave()));
    recalcHeight();
    recalcItemPositions();
}

void SplitTunnelingIpsAndHostnamesItem::validateAndCreateNetworkRoute(QString ipOrHostname)
{
    QString errorTitle = "";
    QString errorDesc = "";

    if (IpValidation::instance().isIpCidrOrDomain(ipOrHostname))
    {
        IpOrHostnameItem * existingItem = itemByName(ipOrHostname);

        if (existingItem)
        {
            errorTitle = "Ip or Hostname already exists";
            errorDesc = "Please enter a value not already in the list.";
        }
        else if (!IpValidation::instance().isValidIpForCidr(ipOrHostname))
        {
            errorTitle = "Incorrect IP/mask combination";
            errorDesc = "Please enter a valid IPv4 address and/or CIDR notation.\n" \
                        "You entered: " + ipOrHostname;
        }
        else if (IpValidation::instance().isWindscribeReservedIp(ipOrHostname))
        {
            errorTitle = "Reserved IP address range";
            errorDesc = ipOrHostname + "is a reserved IP range and can not be changed.";
        }
        else
        {
            ProtoTypes::SplitTunnelingNetworkRouteType type = ProtoTypes::SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME;
            if (IpValidation::instance().isIpCidr(ipOrHostname)) type = ProtoTypes::SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP;

            ProtoTypes::SplitTunnelingNetworkRoute route;
            route.set_name(ipOrHostname.toStdString());
            route.set_type(type);

            createNetworkRouteUiItem(route);
            emitNetworkRoutesUpdated();
        }
    }
    else
    {
        errorTitle = "Invalid Ip or Hostname";
        errorDesc = "Ensure you entered a valid Ip or Hostname like \"1.1.1.1\" or \"something.com\"";
    }

    if (errorTitle != "")
    {
        emit nativeInfoErrorMessage(errorTitle, errorDesc);
    }
}

void SplitTunnelingIpsAndHostnamesItem::onAddNewIpOrHostnameClicked(QString ipOrHostname)
{
    if (loggedIn_)
    {
        validateAndCreateNetworkRoute(ipOrHostname);
    }
    else
    {
        emitNotLoggedInErrorMessage();
    }
}

void SplitTunnelingIpsAndHostnamesItem::emitNotLoggedInErrorMessage()
{
    QString title  = tr("Not Logged In");
    QString desc = tr("Please login to modify rules");
    emit nativeInfoErrorMessage(title, desc);
}

void SplitTunnelingIpsAndHostnamesItem::attemptDelete(IpOrHostnameItem* itemToDelete)
{
    if (loggedIn_)
    {
        int index = selectableIndex(itemToDelete);
        ipsAndHostnames_.removeAll(itemToDelete);
        disconnect(itemToDelete);
        itemToDelete->deleteLater();

        recalcHeight();
        recalcItemPositions();
        QList<ClickableGraphicsObject*> selectable = selectableObjects();
        if (index >= selectable.length()) index = selectable.length()-1;
        if (index >= 0)
        {
            selectable[index]->setSelected(true);
        }
        emitNetworkRoutesUpdated();
    }
    else
    {
        emitNotLoggedInErrorMessage();
    }
}

void SplitTunnelingIpsAndHostnamesItem::updateScaling()
{
    BaseItem::updateScaling();
    recalcHeight();
    recalcItemPositions();
    newIpOrHostnameItem_->setPos(0, 50*G_SCALE);
}

bool SplitTunnelingIpsAndHostnamesItem::hasItemWithFocus()
{
    return newIpOrHostnameItem_->lineEditHasFocus();
}

void SplitTunnelingIpsAndHostnamesItem::onDeleteClicked()
{
    IpOrHostnameItem * itemToDelete = static_cast<IpOrHostnameItem *>(sender());

    attemptDelete(itemToDelete);
}

void SplitTunnelingIpsAndHostnamesItem::onItemHoverLeave()
{
    clearSelection();
}

void SplitTunnelingIpsAndHostnamesItem::onSelectionChanged(bool selected)
{
    if (selected)
    {
        ClickableGraphicsObject *newSelection = static_cast<ClickableGraphicsObject*>(sender());
        clearSelection();
        newSelection->setSelected(true);
    }
}

IpOrHostnameItem *SplitTunnelingIpsAndHostnamesItem::itemByName(QString ipOrHostname) const
{
    IpOrHostnameItem *found = nullptr;
    for (auto *item : ipsAndHostnames_)
    {
        if (item->getIpOrHostnameText() == ipOrHostname)
        {
            found = item;
            break;
        }
    }
    return found;
}

void SplitTunnelingIpsAndHostnamesItem::recalcHeight()
{
    int height = 100;
    height += ipsAndHostnames_.count() * 50;
    setHeight(height*G_SCALE);
}

void SplitTunnelingIpsAndHostnamesItem::recalcItemPositions()
{
    int pos = 100*G_SCALE;
    sortedIpsAndHostnames_ = sorted(ipsAndHostnames_);

    for (auto *item : qAsConst(sortedIpsAndHostnames_))
    {
        item->setPos(0, pos);
        pos += 50*G_SCALE;
    }
}

void SplitTunnelingIpsAndHostnamesItem::emitNetworkRoutesUpdated()
{
    QList<ProtoTypes::SplitTunnelingNetworkRoute> routes;

    for (auto *item : qAsConst(ipsAndHostnames_))
    {
        routes.append(item->getNetworkRoute());
    }

    emit networkRoutesUpdated(routes);
}

QList<IpOrHostnameItem *> SplitTunnelingIpsAndHostnamesItem::sorted(QList<IpOrHostnameItem *> items)
{
    QList<IpOrHostnameItem *> sortedIpOrHostnames;

    for (int i = 0; i < items.length(); i++)
    {
        int insertIndex = 0;
        for (insertIndex = 0; insertIndex < sortedIpOrHostnames.length(); insertIndex++)
        {
            QString loweredInsertItem = sortedIpOrHostnames[insertIndex]->getIpOrHostnameText().toLower();
            if (items[i]->getIpOrHostnameText().toLower() < loweredInsertItem)
            {
                break;
            }
        }
        sortedIpOrHostnames.insert(insertIndex, items[i]);
    }
    return sortedIpOrHostnames;
}

int SplitTunnelingIpsAndHostnamesItem::selectableIndex(ClickableGraphicsObject *object)
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

int SplitTunnelingIpsAndHostnamesItem::selectedObjectIndex()
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

ClickableGraphicsObject *SplitTunnelingIpsAndHostnamesItem::selectedObject()
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

QList<ClickableGraphicsObject *> SplitTunnelingIpsAndHostnamesItem::selectableObjects()
{
    QList<ClickableGraphicsObject*> selectable;

    selectable.append(static_cast<ClickableGraphicsObject *>(newIpOrHostnameItem_));
    for (auto *item : qAsConst(sortedIpsAndHostnames_))
    {
        selectable.append(static_cast<ClickableGraphicsObject*>(item));
    }

    return selectable;
}

void SplitTunnelingIpsAndHostnamesItem::clearSelection()
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
