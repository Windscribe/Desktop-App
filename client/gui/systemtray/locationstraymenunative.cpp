#include "locationstraymenunative.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"
#include "locations/locationsmodel_roles.h"

LocationsTrayMenuNative::LocationsTrayMenuNative(QWidget *parent, QAbstractItemModel *model) :
    QMenu(parent)
{
    // building the menu once in the constructor
    // we do not need to rebuild the menu when the model changes because the menu will rebuild again when the menu is shown again
    // if something changes while the menu is being displayed, it is not critical
    buildMenu(model);
}

void LocationsTrayMenuNative::onMenuActionTriggered(bool checked)
{
    onMenuTriggered(qobject_cast<QAction*>(sender()));
}

void LocationsTrayMenuNative::onMenuTriggered(QAction *action)
{
    WS_ASSERT(action);
    if (!action || !action->isEnabled())
        return;
    emit locationSelected(qvariant_cast<LocationID>(action->data()));
}

void LocationsTrayMenuNative::buildMenu(QAbstractItemModel *model)
{
    clear();
    int rowCount = model->rowCount();
    for (int r = 0; r < rowCount; ++r)
    {
        QModelIndex mi = model->index(r, 0);
        QString countryCode = mi.data(gui_locations::kCountryCode).toString();
        LocationID lid = qvariant_cast<LocationID>(mi.data(gui_locations::kLocationId));
        QSharedPointer<IndependentPixmap> flag;
        if (!lid.isCustomConfigsLocation() && !countryCode.isEmpty()) {
#if defined(Q_OS_MACOS)
            const int flags = ImageResourcesSvg::IMAGE_FLAG_SQUARE;
#else
            const int flags = 0;
#endif
            flag = ImageResourcesSvg::instance().getScaledFlag(countryCode, 20 * G_SCALE, 10 * G_SCALE, flags);
        }

        QString nick = mi.data(gui_locations::kDisplayNickname).toString();
        QString rootVisibleName = mi.data().toString();
        if (!nick.isEmpty()) {
            rootVisibleName += " - " + nick;
        }
        bool bRootShowAsPremium = mi.data(gui_locations::kIsShowAsPremium).toBool();
        if (bRootShowAsPremium) {
            rootVisibleName += " (Pro)";
        }

        int childsCount = model->rowCount(mi);
        if (childsCount == 0 || bRootShowAsPremium)
        {
            QAction *action = addAction(rootVisibleName);
            connect(action, &QAction::triggered, this, &LocationsTrayMenuNative::onMenuActionTriggered);
            if (flag) {
                action->setIconVisibleInMenu(true);
                action->setIcon(flag->getIcon());
            }

            QVariant stored;
            stored.setValue(lid);
            action->setData(stored);
            action->setEnabled(!bRootShowAsPremium && !mi.data(gui_locations::kIsDisabled).toBool());
        }
        else
        {
            QMenu *subMenu = addMenu(mi.data().toString());
            connect(subMenu, &QMenu::triggered, this, &LocationsTrayMenuNative::onMenuTriggered);
            subMenu->setEnabled(!mi.data(gui_locations::kIsDisabled).toBool());
            if (flag) {
                subMenu->menuAction()->setIconVisibleInMenu(true);
                subMenu->setIcon(flag->getIcon());
            }

            for (int cityInd = 0; cityInd < childsCount; ++cityInd)
            {
                QModelIndex cityMi = model->index(cityInd, 0, mi);
                QString visibleName = cityMi.data().toString() + " - " + cityMi.data(gui_locations::kDisplayNickname).toString();
                if (cityMi.data(gui_locations::kIsShowAsPremium).toBool()) {
                    visibleName += " (Pro)";
                }
                QAction *cityAction = subMenu->addAction(visibleName);
                cityAction->setEnabled(!cityMi.data(gui_locations::kIsDisabled).toBool());
                cityAction->setData(cityMi.data(gui_locations::kLocationId));
            }
        }
    }
}
