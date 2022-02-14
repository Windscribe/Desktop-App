#ifndef SPLITTUNNELINGAPPSSEARCHITEM_H
#define SPLITTUNNELINGAPPSSEARCHITEM_H

#include "../baseitem.h"
#include "searchlineedititem.h"
#include "appsearchitem.h"

namespace PreferencesWindow {

class SplitTunnelingAppsSearchItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsSearchItem(ScalableGraphicsObject * parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QList<ProtoTypes::SplitTunnelingApp> getApps();
    void setApps(QList<ProtoTypes::SplitTunnelingApp> apps);
    void setFocusOnSearchBar();
    void setLoggedIn(bool loggedIn);

    void toggleAppItemActive(AppSearchItem *item);
    void updateProgramList();

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void appsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);
    void searchModeExited();
    void nativeInfoErrorMessage(QString,QString);
    void scrollToRect(QRect r);

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onSearchTextChanged(QString text);
    void onAppItemClicked();
    void onItemHoverLeave();
    void onItemHoverEnter();
    void onSearchBoxEnterPressed();
    void onSearchBoxFocusIn();

private:
    bool loggedIn_;
    SearchLineEditItem *searchLineEditItem_;

    QList<ProtoTypes::SplitTunnelingApp> apps_;
    QList<ProtoTypes::SplitTunnelingApp> filteredAppItems(QString filterString);
    ProtoTypes::SplitTunnelingApp *appByName(QString appName);

    QList<QSharedPointer<AppSearchItem>> drawnApps_;
    void updateDrawBasedOnFilterText(QString filterText);
    void drawItemsAndUpdateHeight(int baseHeight, QList<ProtoTypes::SplitTunnelingApp> itemsToDraw);
    void updateItemsPosAndUpdateHeight();

    void removeAppFromApps(ProtoTypes::SplitTunnelingApp app);

    QList<ProtoTypes::SplitTunnelingApp> systemApps_;
    void updateSystemApps();
    QList<ProtoTypes::SplitTunnelingApp> activeAndSystemApps();
    void emitNotLoggedInErrorMessage();

    int selectableIndex(ClickableGraphicsObject *object);
    int selectedObjectIndex();
    ClickableGraphicsObject *selectedObject();
    QList<ClickableGraphicsObject*> selectableObjects();
    void clearSelection();
};

}

#endif // SPLITTUNNELINGAPPSSEARCHITEM_H
