#include "splittunnelingswitchitem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include "utils/logger.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"


#ifdef Q_OS_MAC
#include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingSwitchItem::SplitTunnelingSwitchItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50*G_SCALE)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    activeCheckBox_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Split Tunneling"), QString());
    bool splitTunnelingActive = true; // overwritten by first setSettings call
    activeCheckBox_->setState(splitTunnelingActive);
    activeCheckBox_->setLineVisible(false);
    connect(activeCheckBox_, SIGNAL(stateChanged(bool)), SLOT(onActiveSwitchChanged(bool)));
    connect(activeCheckBox_, SIGNAL(buttonHoverLeave()), SLOT(onActiveSwitchHoverLeave()));

    int modeHeight = 43;
    modeComboBox_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Mode"), "", modeHeight, FontManager::instance().getMidnightColor(), 24, true);
    modeComboBox_->setCaptionFont(FontDescr(12, false));
    reloadModeComboBox();
    modeComboBox_->setCurrentItem(0); // overwritten by first setSettings call
    connect(modeComboBox_, SIGNAL(currentItemChanged(QVariant)), SLOT(onCurrentModeChanged(QVariant)));

    appsSubPageItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","Apps"), true);
    connect(appsSubPageItem_, SIGNAL(clicked()), SIGNAL(appsPageClick()));

    ipsAndHostnamesSubPageItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","IPs & Hostnames"), true);
    connect(ipsAndHostnamesSubPageItem_, SIGNAL(clicked()), SIGNAL(ipsAndHostnamesPageClick()));

    modeInfo_ = new IconButton(16, 16, "INFO_ICON", "", this, 0.25, 0.25);
    modeInfo_->setClickableHoverable(false, true);
    connect(modeInfo_, SIGNAL(hoverEnter()), SLOT(onModeInfoHoverEnter()));
    connect(modeInfo_, SIGNAL(hoverLeave()), SLOT(onModeInfoHoverLeave()));

    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);

    appsSubPageItem_->setArrowText("");
    ipsAndHostnamesSubPageItem_->setArrowText("");

    setHeight(COLLAPSED_HEIGHT);
    updatePositions();
}

void SplitTunnelingSwitchItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void SplitTunnelingSwitchItem::setSettings(types::SplitTunnelingSettings settings)
{
#ifdef Q_OS_MAC
    if (settings.active() && MacUtils::isOsVersionIsBigSur_or_greater()) {
        settings.set_active(false);
        emit settingsChanged(settings);
    }
#endif  // Q_OS_MAC
    settings_ = settings;
    updateActiveUI(settings.active);
    updateModeUI(settings.mode);
}

void SplitTunnelingSwitchItem::onActiveSwitchChanged(bool checked)
{
#ifdef Q_OS_MAC
    if (checked && MacUtils::isOsVersionIsBigSur_or_greater())
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(
            view->mapFromScene(activeCheckBox_->getButtonScenePos()));

        TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_SPLIT_ROUTING_UNSUPPORTED);
        ti.x = globalPt.x() + 15 * G_SCALE;
        ti.y = globalPt.y() - 4 * G_SCALE;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.95;
        ti.title = tr("Unsupported");
        ti.desc = tr("Sorry, Split Tunneling is currently not supported on your operating system."
                     " We are working hard to enable the feature as soon as possible.");
        ti.width = 260 * G_SCALE;
        ti.delay = 100;

        QTimer::singleShot(0, [ti]() { TooltipController::instance().showTooltipDescriptive(ti); });

        activeCheckBox_->setState(false);
        checked = false;
    }
#endif  // Q_OS_MAC
    if (settings_.active != checked) {
        settings_.active = checked;
        updateActiveUI(checked);
        emit settingsChanged(settings_);
    }
}

void SplitTunnelingSwitchItem::onActiveSwitchHoverLeave()
{
#ifdef Q_OS_MAC
    TooltipController::instance().hideTooltip(TOOLTIP_ID_SPLIT_ROUTING_UNSUPPORTED);
#endif  // Q_OS_MAC
}

void SplitTunnelingSwitchItem::onExpandAnimationValueChanged(QVariant value)
{
    setHeight(value.toInt()*G_SCALE);
}

void SplitTunnelingSwitchItem::onCurrentModeChanged(QVariant value)
{
    SPLIT_TUNNELING_MODE mode = static_cast<SPLIT_TUNNELING_MODE>(value.toInt());
    settings_.mode = mode;
    emit settingsChanged(settings_);
}

void SplitTunnelingSwitchItem::onModeInfoHoverEnter()
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(modeInfo_->scenePos()));

    int width = 150 * G_SCALE;
    int posX = globalPt.x() + 27 * G_SCALE;
    int posY = globalPt.y() +  7 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_SPLIT_ROUTING_MODE_INFO);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_LEFT;
    ti.tailPosPercent = 0.05;
    ti.title = tr("Mode");
    ti.desc = tr("Exclusive: Selected apps and hostnames will not use VPN connection. \n\nInclusive: Only selected apps and hostnames will use VPN connection.");
    ti.width = width;
    TooltipController::instance().showTooltipDescriptive(ti);
}

void SplitTunnelingSwitchItem::onModeInfoHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_SPLIT_ROUTING_MODE_INFO);
}

void SplitTunnelingSwitchItem::setClickable(bool clickable)
{
    modeInfo_->setClickableHoverable(false, clickable);
}

void SplitTunnelingSwitchItem::setAppsCount(int count)
{
    appsSubPageItem_->setArrowText(QString("%1").arg(count));
}

void SplitTunnelingSwitchItem::setIpsAndHostnamesCount(int count)
{
    ipsAndHostnamesSubPageItem_->setArrowText(QString("%1").arg(count));
}

void SplitTunnelingSwitchItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(activeCheckBox_->isChecked() ? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    updatePositions();
}

void SplitTunnelingSwitchItem::reloadModeComboBox()
{
    modeComboBox_->clear();
    modeComboBox_->addItem(tr("Exclusive"), SPLIT_TUNNELING_MODE_EXCLUDE);
    modeComboBox_->addItem(tr("Inclusive"), SPLIT_TUNNELING_MODE_INCLUDE);
}

void SplitTunnelingSwitchItem::updateActiveUI(bool checked)
{
    activeCheckBox_->setState(checked);

    if (checked)
    {
        // expand animation
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }

        //qCDebug(LOG_USER) << "Enabled Split Tunneling";
    }
    else
    {
        // collapse animation
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }

        //qCDebug(LOG_USER) << "Disabled Split Tunneling";
    }
}

void SplitTunnelingSwitchItem::updateModeUI(SPLIT_TUNNELING_MODE mode)
{
    if (modeComboBox_->hasItems())
    {
        modeComboBox_->setCurrentItem(mode);
    }
}

void SplitTunnelingSwitchItem::updatePositions()
{
    int modeHeight = 43;
    modeComboBox_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    appsSubPageItem_->setPos(0, (COLLAPSED_HEIGHT + modeHeight)*G_SCALE);
    ipsAndHostnamesSubPageItem_->setPos(0, (COLLAPSED_HEIGHT + modeHeight + 43)*G_SCALE);
    modeInfo_->setPos(77*G_SCALE, 55*G_SCALE);

}

}
