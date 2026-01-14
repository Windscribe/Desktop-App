#include "appincludeditem.h"

#include <QIcon>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"
#include "widgetutils/widgetutils.h"
#include "dpiscalemanager.h"

#if defined(Q_OS_MACOS)
#include "utils/macutils.h"
#endif

namespace PreferencesWindow {

AppIncludedItem::AppIncludedItem(types::SplitTunnelingApp app, ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), app_(app)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    toggleButton_ = new ToggleButton(this);
    toggleButton_->setState(app_.active);
    connect(toggleButton_, &ToggleButton::stateChanged, this, &AppIncludedItem::onToggleChanged);

    deleteButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/DELETE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    deleteButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    deleteButton_->setHoverOpacity(OPACITY_FULL);
    connect(deleteButton_, &IconButton::clicked, this, &AppIncludedItem::deleteClicked);

    if (app_.icon.isEmpty()) {
#if defined(Q_OS_WIN)
        app_.icon = app_.fullName;
#elif defined(Q_OS_MACOS)
        app_.icon = MacUtils::iconPathFromBinPath(app_.fullName);
#endif
    }

    updatePositions();
}

void AppIncludedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // app icon
#if !defined(Q_OS_LINUX)
    painter->save();
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIconIndependentPixmap(app_.icon);
    if (!p) {
        p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    }
    p->draw(PREFERENCES_MARGIN_X*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE, painter);
    painter->restore();
#else
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_QUESTION_MARK_ICON");
    QPixmap pixmap = QIcon::fromTheme(app_.icon, p->getIcon()).pixmap(QSize(APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE));

    painter->drawPixmap(PREFERENCES_MARGIN_X*G_SCALE, APP_ICON_MARGIN_Y*G_SCALE, pixmap);
#endif

    // app name
    painter->setOpacity(OPACITY_FULL * initOpacity);
    painter->setPen(Qt::white);
    QFont font = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font);
    QFontMetrics fm(font);
    QString elidedName = fm.elidedText(app_.name,
                                       Qt::TextElideMode::ElideRight,
                                       boundingRect().width() - (4*PREFERENCES_MARGIN_X + APP_ICON_MARGIN_X + APP_ICON_WIDTH + ICON_WIDTH)*G_SCALE - toggleButton_->boundingRect().width());
    painter->drawText(boundingRect().adjusted((PREFERENCES_MARGIN_X + APP_ICON_WIDTH + APP_ICON_MARGIN_X)*G_SCALE,
                                              0,
                                              -(3*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE - toggleButton_->boundingRect().width(),
                                              0),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      elidedName);
}

QString AppIncludedItem::getName()
{
    return app_.name;
}

QString AppIncludedItem::getAppIcon()
{
    return app_.icon;
}

bool AppIncludedItem::isActive()
{
    return app_.active;
}

void AppIncludedItem::onToggleChanged(bool checked)
{
    app_.active = checked;
    emit activeChanged(checked);
}

void AppIncludedItem::setSelected(bool selected)
{
    selected_ = selected;
    deleteButton_->setSelected(selected);
    update();
}

void AppIncludedItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void AppIncludedItem::updatePositions()
{
    deleteButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2);
    toggleButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE - toggleButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE,
                          (PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE - toggleButton_->boundingRect().height()) / 2);
}

void AppIncludedItem::setClickable(bool clickable)
{
    deleteButton_->setClickable(clickable);
}

}
