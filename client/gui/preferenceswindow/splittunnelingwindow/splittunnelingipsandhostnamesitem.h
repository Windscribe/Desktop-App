#ifndef SPLITTUNNELINGIPSANDHOSTNAMESITEM_H
#define SPLITTUNNELINGIPSANDHOSTNAMESITEM_H

#include "../baseitem.h"
#include "newiporhostnameitem.h"
#include "iporhostnameitem.h"

namespace PreferencesWindow {

class SplitTunnelingIpsAndHostnamesItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingIpsAndHostnamesItem(ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setNetworkRoutes(QList<types::SplitTunnelingNetworkRoute> routes);
    void setLoggedIn(bool loggedIn);
    void setFocusOnTextEntry();

    void attemptDelete(IpOrHostnameItem *itemToDelete);

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void escape();
    void networkRoutesUpdated(QList<types::SplitTunnelingNetworkRoute> routes);
    void nativeInfoErrorMessage(QString title, QString desc);
    void scrollToRect(QRect r);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void onAddNewIpOrHostnameClicked(QString ipOrHostname);
    void onDeleteClicked();

    void onItemHoverLeave();
    void onSelectionChanged(bool selected);

private:
    bool loggedIn_;
    NewIpOrHostnameItem *newIpOrHostnameItem_;
    QList<IpOrHostnameItem *> ipsAndHostnames_;
    QList<IpOrHostnameItem *> sortedIpsAndHostnames_;

    void validateAndCreateNetworkRoute(QString ipOrHostname);
    void createNetworkRouteUiItem(types::SplitTunnelingNetworkRoute ipOrHostname);

    IpOrHostnameItem *itemByName(QString ipOrHostname) const;
    void recalcHeight();
    void recalcItemPositions();

    void emitNetworkRoutesUpdated();
    void emitNotLoggedInErrorMessage();

    QList<IpOrHostnameItem *> sorted(QList<IpOrHostnameItem *> items);

    int selectableIndex(ClickableGraphicsObject *object);
    int selectedObjectIndex();
    ClickableGraphicsObject *selectedObject();
    QList<ClickableGraphicsObject*> selectableObjects();
    void clearSelection();

};

}
#endif // SPLITTUNNELINGIPSANDHOSTNAMESITEM_H
