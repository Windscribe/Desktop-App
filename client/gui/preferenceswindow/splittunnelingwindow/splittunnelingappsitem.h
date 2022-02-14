#ifndef SPLITTUNNELINGAPPSITEM_H
#define SPLITTUNNELINGAPPSITEM_H

#include <QSharedPointer>
#include "../baseitem.h"
#include "searchlineedititem.h"
#include "appincludeditem.h"
#include "preferenceswindow/dividerline.h"

namespace PreferencesWindow {

class SplitTunnelingAppsItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingAppsItem(ScalableGraphicsObject *parent);
    ~SplitTunnelingAppsItem();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QList<ProtoTypes::SplitTunnelingApp> getApps();
    void setApps(QList<ProtoTypes::SplitTunnelingApp> processNames);
    void setLoggedIn(bool loggedIn);

    void attemptDelete(AppIncludedItem *appItem);

    void attemptAdd();
    void updateScaling() override;

signals:
    void escape();
    void searchClicked();
    void addClicked();
    void appsUpdated(QList<ProtoTypes::SplitTunnelingApp> apps);
    void nativeInfoErrorMessage(QString title, QString desc);
    void scrollToRect(QRect r);

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onAppDeleteClicked();
    void onAddClicked();
    void onSelectionChanged(bool selected);
    void onButtonHoverEnter();
    void onItemHoverLeave();

private:
    bool loggedIn_;
    bool atLeastOneApp_;
    IconButton *searchButton_;
    IconButton *addButton_;
    DividerLine *dividerLine_;

    QList<ProtoTypes::SplitTunnelingApp> apps_; // only active apps
    QList<QSharedPointer<AppIncludedItem>> drawnApps_;

    void drawItemsAndUpdateHeight();
    void updateItemsPosAndUpdateHeight();
    ProtoTypes::SplitTunnelingApp *appByName(QString appName);

    void removeAppFromApps(ProtoTypes::SplitTunnelingApp app);
    void emitNotLoggedInErrorMessage();

    int selectableIndex(ClickableGraphicsObject *object);
    int selectedObjectIndex();
    ClickableGraphicsObject *selectedObject();
    QList<ClickableGraphicsObject*> selectableObjects();
    void clearSelection();
    void updatePositions();
};

}

#endif // SPLITTUNNELINGAPPSITEM_H
