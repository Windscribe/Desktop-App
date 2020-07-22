#include "automanualswitchitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "GraphicResources/imageresourcessvg.h"
#include "GraphicResources/fontmanager.h"
#include "CommonGraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AutoManualSwitchItem::AutoManualSwitchItem(ScalableGraphicsObject *parent, const QString &caption) : BaseItem(parent, 50),
    strCaption_(caption), isExpanded_(false), linePos_(0.0)
{
    btnAuto_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Auto"), FontDescr(12, true), QColor(255, 255, 255), true, this);
    connect(btnAuto_, SIGNAL(clicked()), SLOT(onClickAuto()));
    btnManual_ = new CommonGraphics::TextButton(QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Manual"), FontDescr(12, true), QColor(255, 255, 255), true, this);
    connect(btnManual_, SIGNAL(clicked()), SLOT(onClickManual()));

    btnAuto_->setClickable(false);
    btnAuto_->setCurrentOpacity(OPACITY_FULL);
    btnAuto_->setUnhoverOpacity(OPACITY_UNHOVER_TEXT);
    btnManual_->setClickable(true);
    btnManual_->setUnhoverOpacity(OPACITY_UNHOVER_TEXT);

    anim_.setStartValue(0.0);
    anim_.setEndValue(1.0);
    anim_.setDuration(150);
    connect(&anim_, SIGNAL(valueChanged(QVariant)), SLOT(onAnimationValueChanged(QVariant)));

    updatePositions();
}

void AutoManualSwitchItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // draw divider line
    QRectF rcDivider(24*G_SCALE, boundingRect().bottom() - 3*G_SCALE, 276*G_SCALE, 3*G_SCALE);
    painter->fillRect(rcDivider.adjusted(0, 0, 0, -2*G_SCALE), QBrush(QColor(13, 18, 36)));
    painter->fillRect(rcDivider.adjusted(0, 1*G_SCALE, 0, -1*G_SCALE), QBrush(QColor(40, 45, 61)));
    painter->fillRect(rcDivider.adjusted(0, 2*G_SCALE, 0, 0), QBrush(QColor(30, 36, 52)));

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr(strCaption_.toStdString().c_str()));

    drawSelectionLine(painter, linePos_);
}

void AutoManualSwitchItem::setState(SWITCH_STATE state)
{
    if (state == AUTO)
    {
        isExpanded_ = false;
        linePos_ = 0.0;
        btnAuto_->setCurrentOpacity(OPACITY_FULL);
        btnAuto_->setClickable(false);

        btnManual_->setClickable(true);
        btnManual_->setCurrentOpacity(OPACITY_UNHOVER_TEXT);
        update();
    }
    else
    {
        isExpanded_ = true;
        linePos_ = 1.0;
        btnAuto_->setClickable(true);
        btnAuto_->setCurrentOpacity(OPACITY_UNHOVER_TEXT);

        btnManual_->setCurrentOpacity(OPACITY_FULL);
        btnManual_->setClickable(false);
        update();
    }
}

void AutoManualSwitchItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();
}

void AutoManualSwitchItem::onClickAuto()
{
    if (isExpanded_)
    {
        anim_.setDirection(QVariantAnimation::Backward);
        if (anim_.state() != QVariantAnimation::Running)
        {
            anim_.start();
        }
        isExpanded_ = false;

        btnAuto_->setCurrentOpacity(OPACITY_FULL);
        btnAuto_->setClickable(false);

        btnManual_->setClickable(true);
        btnManual_->setCurrentOpacity(OPACITY_UNHOVER_TEXT);

        emit stateChanged(AUTO);
    }
}

void AutoManualSwitchItem::onClickManual()
{
    if (!isExpanded_)
    {
        anim_.setDirection(QVariantAnimation::Forward);
        if (anim_.state() != QVariantAnimation::Running)
        {
            anim_.start();
        }
        isExpanded_ = true;

        btnAuto_->setClickable(true);
        btnAuto_->setCurrentOpacity(OPACITY_UNHOVER_TEXT);

        btnManual_->setCurrentOpacity(OPACITY_FULL);
        btnManual_->setClickable(false);

        emit stateChanged(MANUAL);
    }
}

void AutoManualSwitchItem::onAnimationValueChanged(const QVariant &value)
{
    linePos_ = value.toReal();
    update();
}

void AutoManualSwitchItem::drawSelectionLine(QPainter *painter, qreal v)
{
    int x1 = btnAuto_->pos().x();
    int x2 = btnManual_->pos().x();
    int curX = (x2 - x1) * v + x1;

    int w1 = btnAuto_->boundingRect().width();
    int w2 = btnManual_->boundingRect().width();
    int curWidth = (w2 - w1) * v + w1 + 2*G_SCALE;

    int nextOffs = 1*G_SCALE;
    painter->fillRect(curX, boundingRect().bottom() - nextOffs, curWidth, 1*G_SCALE, QBrush(QColor(195, 197, 201)));
    nextOffs += 1*G_SCALE;
    painter->fillRect(curX, boundingRect().bottom() - nextOffs, curWidth, 1*G_SCALE, QBrush(QColor(255, 255, 255)));
    nextOffs += 1*G_SCALE;
    painter->fillRect(curX, boundingRect().bottom() - nextOffs, curWidth, 1*G_SCALE, QBrush(QColor(66, 70, 85)));
}

void AutoManualSwitchItem::updatePositions()
{
    btnManual_->recalcBoundingRect();
    btnAuto_->recalcBoundingRect();
    btnManual_->setPos(boundingRect().right() - 16*G_SCALE - btnManual_->boundingRect().width(), (boundingRect().height() - btnManual_->boundingRect().height()) / 2);
    btnAuto_->setPos(btnManual_->pos().x() - 16*G_SCALE - btnAuto_->boundingRect().width(), (boundingRect().height() - btnAuto_->boundingRect().height()) / 2);
}


} // namespace PreferencesWindow
