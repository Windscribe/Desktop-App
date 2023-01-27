#include "firewallmodeitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "utils/protoenumtostring.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

FirewallModeItem::FirewallModeItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50),
    isExpanded_(false), curDescrTextOpacity_(0.0)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxFirewallMode_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Firewall Mode"), "", 50, /*QColor(255, 0, 0)*/Qt::transparent, 0, false);

    const QList< QPair<QString, int> > modes = ProtoEnumToString::instance().getEnums(ProtoTypes::FirewallMode_descriptor());
    for (const auto &v : modes)
    {
        comboBoxFirewallMode_->addItem(v.first, v.second);
    }
    comboBoxFirewallMode_->setCurrentItem(curFirewallMode_.mode());
    connect(comboBoxFirewallMode_, SIGNAL(currentItemChanged(QVariant)), SLOT(onFirewallModeChanged(QVariant)));
    connect(comboBoxFirewallMode_, SIGNAL(buttonHoverEnter()), SIGNAL(buttonHoverEnter()));
    connect(comboBoxFirewallMode_, SIGNAL(buttonHoverLeave()), SIGNAL(buttonHoverLeave()));

    comboBoxFirewallWhen_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "When?"), "", 43, QColor(16, 22, 40), 24, true);

    const QList< QPair<QString, int> > whens = ProtoEnumToString::instance().getEnums(ProtoTypes::FirewallWhen_descriptor());
    for (const auto &v : whens)
    {
        comboBoxFirewallWhen_->addItem(v.first, v.second);
    }
    comboBoxFirewallWhen_->setCurrentItem(curFirewallMode_.when());


    connect(comboBoxFirewallWhen_, SIGNAL(currentItemChanged(QVariant)), SLOT(onFirewallWhenChanged(QVariant)));

    comboBoxFirewallWhen_->setPos(0, COLLAPSED_HEIGHT + 38);

    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 50 - dividerLine_->boundingRect().height());

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
}

void FirewallModeItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt()* G_SCALE);

    qreal progress = (qreal)(value.toInt() - COLLAPSED_HEIGHT) / (qreal)(EXPANDED_HEIGHT - COLLAPSED_HEIGHT);
    curDescrTextOpacity_ = progress;
    dividerLine_->setPos(24 * G_SCALE, value.toInt() * G_SCALE - dividerLine_->boundingRect().height());
}

void FirewallModeItem::onLanguageChanged()
{
    // Mode
    QVariant mode = comboBoxFirewallMode_->currentItem();
    comboBoxFirewallMode_->clear();

    /*const QList<FirewallModeType> modes = FirewallModeType::allAvailableTypes();
    for (const FirewallModeType &v : modes)
    {
        comboBoxFirewallMode_->addItem(v.toString(), (int)v.type());
    }
    comboBoxFirewallMode_->setCurrentItem(mode.toInt());

    // When
    QVariant when = comboBoxFirewallWhen_->currentItem();
    comboBoxFirewallWhen_->clear();

    const QList<FirewallWhenType> whens = FirewallWhenType::allAvailableTypes();
    for (const FirewallWhenType &w : whens)
    {
        comboBoxFirewallWhen_->addItem(w.toString(), (int)w.type());
    }
    comboBoxFirewallWhen_->setCurrentItem(when.toInt());*/
}

void FirewallModeItem::hideOpenPopups()
{
    comboBoxFirewallMode_->hideMenu();
    comboBoxFirewallWhen_->hideMenu();
}

void FirewallModeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setPen(QColor(255, 255, 255));
    QRectF bottomHalfRect = boundingRect().adjusted(16*G_SCALE, 44*G_SCALE, -40*G_SCALE, 0);

    painter->setOpacity(0.5 * curDescrTextOpacity_ * initOpacity);
    QFont *fontSmall = FontManager::instance().getFont(12, true);
    painter->setFont(*fontSmall);
    painter->drawText(bottomHalfRect, tr("Firewall is handled automatically by Windscribe's connection status."));
}

void FirewallModeItem::setFirewallMode(const ProtoTypes::FirewallSettings &fm)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curFirewallMode_, fm))
    {
        curFirewallMode_ = fm;
        if (curFirewallMode_.mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC)
        {
            isExpanded_ = true;
            onExpandAnimationValueChanged(EXPANDED_HEIGHT);
        }
        else
        {
            isExpanded_ = false;
            onExpandAnimationValueChanged(COLLAPSED_HEIGHT);
        }
        comboBoxFirewallMode_->setCurrentItem(curFirewallMode_.mode());
        comboBoxFirewallWhen_->setCurrentItem(curFirewallMode_.when());
    }
}

void FirewallModeItem::setFirewallBlock(bool isFirewallBlocked)
{
    comboBoxFirewallMode_->setClickable(!isFirewallBlocked);
}

QPointF FirewallModeItem::getButtonScenePos() const
{
    return comboBoxFirewallMode_->getButtonScenePos();
}

void FirewallModeItem::updateScaling()
{
    BaseItem::updateScaling();

    //update state/positions
    onExpandAnimationValueChanged(isExpanded_ ? EXPANDED_HEIGHT : COLLAPSED_HEIGHT);
    comboBoxFirewallWhen_->setPos(0, (COLLAPSED_HEIGHT + 38)*G_SCALE);
}

void FirewallModeItem::onFirewallModeChanged(QVariant v)
{
    if (v.toInt() == ProtoTypes::FIREWALL_MODE_AUTOMATIC && !isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;
    }
    else if (isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;
    }

    if (curFirewallMode_.mode() != v.toInt())
    {
        curFirewallMode_.set_mode((ProtoTypes::FirewallMode)v.toInt());
        emit firewallModeChanged(curFirewallMode_);
    }
}

void FirewallModeItem::onFirewallWhenChanged(QVariant v)
{
    if (curFirewallMode_.when() != v.toInt())
    {
        curFirewallMode_.set_when((ProtoTypes::FirewallWhen)v.toInt());
        emit firewallModeChanged(curFirewallMode_);
    }
}


} // namespace PreferencesWindow
