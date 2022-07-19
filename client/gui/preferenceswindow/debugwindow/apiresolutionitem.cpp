#include "apiresolutionitem.h"

#include <QPainter>
#include "../dividerline.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ApiResolutionItem::ApiResolutionItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50),
    isExpanded_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    switchItem_ = new AutoManualSwitchItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::AutoManualSwitchItem", "API Resolution"));
    connect(switchItem_, SIGNAL(stateChanged(AutoManualSwitchItem::SWITCH_STATE)), SLOT(onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE)));
    switchItem_->setPos(0, 0);

    editBoxIP_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter IP"), false);
    connect(editBoxIP_, SIGNAL(textChanged(QString)), SLOT(onIPChanged(QString)));

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxIP_->setValidator(ipValidator);

    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));

    updatePositions();
}

void ApiResolutionItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt());
}

void ApiResolutionItem::updatePositions()
{
    editBoxIP_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setStartValue(COLLAPSED_HEIGHT*G_SCALE);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT*G_SCALE);
}

void ApiResolutionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void ApiResolutionItem::setApiResolution(const ProtoTypes::ApiResolution &ar)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curApiResolution_, ar))
    {
        curApiResolution_ = ar;
        expandEnimation_.stop();

        if (ar.is_automatic())
        {
            switchItem_->setState(AutoManualSwitchItem::AUTO);
            isExpanded_ = false;
            setHeight(COLLAPSED_HEIGHT*G_SCALE);
        }
        else
        {
            editBoxIP_->setText(QString::fromStdString(ar.manual_ip()));
            switchItem_->setState(AutoManualSwitchItem::MANUAL);
            isExpanded_ = true;
            setHeight(EXPANDED_HEIGHT*G_SCALE);
        }
    }
}

void ApiResolutionItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(isExpanded_ ? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    updatePositions();
}

bool ApiResolutionItem::hasItemWithFocus()
{
    return editBoxIP_->lineEditHasFocus();
}

void ApiResolutionItem::onSwitchChanged(AutoManualSwitchItem::SWITCH_STATE state)
{
    if (state == AutoManualSwitchItem::MANUAL && !isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;

        curApiResolution_.set_is_automatic(false);
        emit apiResolutionChanged(curApiResolution_);
    }
    else if (isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;

        curApiResolution_.set_is_automatic(true);
        emit apiResolutionChanged(curApiResolution_);
    }
}

void ApiResolutionItem::onIPChanged(const QString &text)
{
    curApiResolution_.set_manual_ip(text.trimmed().toStdString());
    emit apiResolutionChanged(curApiResolution_);
}


} // namespace PreferencesWindow
