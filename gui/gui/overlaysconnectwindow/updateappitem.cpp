#include "updateappitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace UpdateApp {

UpdateAppItem::UpdateAppItem(QGraphicsObject *parent) : ScalableGraphicsObject(parent),
    mode_(UPDATE_APP_ITEM_MODE_PROMPT), curVersionText_(""), curBackgroundOpacity_(OPACITY_FULL),
    curVersionOpacity_(OPACITY_FULL), curProgressBackgroundOpacity_(OPACITY_HIDDEN),
    curProgressForegroundOpacity_(OPACITY_HIDDEN), curProgressBarPos_(0)
{
    QString updateText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "UPDATE");
    updateButton_ = new CommonGraphics::TextButton(updateText, FontDescr(11, true, 105),
                                                   Qt::white, true, this, 15);

    connect(updateButton_, SIGNAL(clicked()), SLOT(onUpdateClick()));

    updateButton_->animateShow(ANIMATION_SPEED_FAST);

    // Update button text-hover-button (hideable)
    connect(&backgroundOpacityAnimation_        , SIGNAL(valueChanged(QVariant)), this, SLOT(onBackgroundOpacityChanged(QVariant)));
    connect(&versionOpacityAnimation_           , SIGNAL(valueChanged(QVariant)), this, SLOT(onVersionOpacityChanged(QVariant)));
    connect(&progressForegroundOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onProgressForegroundOpacityChanged(QVariant)));
    connect(&progressBackgroundOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onProgressBackgroundOpacityChanged(QVariant)));

    connect(&progressBarPosChangeAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onProgressBarPosChanged(QVariant)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
    updatePositions();
}

QGraphicsObject *UpdateAppItem::getGraphicsObject()
{
    return this;
}

QRectF UpdateAppItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, HEIGHT*G_SCALE);
}

void UpdateAppItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setRenderHint(QPainter::Antialiasing);

    // background
    painter->save();
    painter->setBrush(FontManager::instance().getMidnightColor());
    painter->setPen(FontManager::instance().getMidnightColor());
    painter->setOpacity(curBackgroundOpacity_ * initOpacity);
    QRectF rect(1*G_SCALE, 1*G_SCALE, (WIDTH-2)*G_SCALE, (HEIGHT - 2)*G_SCALE);
    painter->drawRoundedRect(rect, 10, 100, Qt::RelativeSize);
    painter->restore();

    // icon
    painter->save();
    painter->setOpacity(curVersionOpacity_ * initOpacity);
    IndependentPixmap * pixmap = ImageResourcesSvg::instance().getIndependentPixmap("update/INFO_ICON");
    pixmap->draw(2*G_SCALE, 2*G_SCALE, painter);
    painter->restore();

    // version text
    painter->save();
    QFont *font = FontManager::instance().getFont(11,true, 105);
    QFontMetrics fm(*font);
    painter->translate(0, VERSION_TEXT_HEIGHT*G_SCALE);
    painter->setPen(FontManager::instance().getBrightYellowColor());
    painter->setFont(*font);
    painter->setOpacity(curVersionOpacity_ * initOpacity);
    painter->drawText(25*G_SCALE, (HEIGHT*G_SCALE - fm.height() - fm.ascent() / 2) - (G_SCALE+0.5), curVersionText_);
    painter->restore();

    painter->save();
    painter->setBrush(FontManager::instance().getBrightYellowColor());

    int xRadius = 6;
    int yRadius = 100;
    int endPosXOffset = 13;
    int barWidth = WIDTH - endPosXOffset;

    // background progress bar (rounded rect)
    painter->setOpacity(curProgressBackgroundOpacity_ * initOpacity);
    QRectF backgroundProgressRect(5*G_SCALE,5*G_SCALE, barWidth*G_SCALE, (HEIGHT-10)*G_SCALE);
    painter->drawRoundedRect(backgroundProgressRect, xRadius*G_SCALE, yRadius);

    int foregroundRectWidth = barWidth*((double) curProgressBarPos_ /100);
    if (foregroundRectWidth > 0)
    {
        // forground progress bar (rounded rect)
        painter->setOpacity(curProgressForegroundOpacity_ * initOpacity);
        QRectF foregroundProgressRect(5*G_SCALE,5*G_SCALE, foregroundRectWidth*G_SCALE, (HEIGHT-10)*G_SCALE);
        painter->drawRoundedRect(foregroundProgressRect, xRadius*G_SCALE, yRadius);
    }
    painter->restore();

}

void UpdateAppItem::setVersionAvailable(const QString &versionNumber,int buildNumber)
{
    QString prefix = "";
    if (versionNumber != "")
    {
        if (versionNumber.at(0) != 'v')
        {
            prefix = tr("v");
        }
    }

    curVersionText_ = prefix + versionNumber + "." + QString::number(buildNumber);

    setMode(UPDATE_APP_ITEM_MODE_PROMPT);
    update();
}

void UpdateAppItem::setProgress(int value)
{
    if (mode_ == UPDATE_APP_ITEM_MODE_PROGRESS)
    {
        startAnAnimation<int>(progressBarPosChangeAnimation_, curProgressBarPos_, value, ANIMATION_SPEED_VERY_FAST);
    }
}

QPixmap UpdateAppItem::getCurrentPixmapShape()
{
    QPixmap pixmap(WIDTH*G_SCALE, HEIGHT*G_SCALE);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setClipRect(0, 0, WIDTH*G_SCALE, HEIGHT / 2 * G_SCALE);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(FontManager::instance().getMidnightColor());
    painter.setPen(FontManager::instance().getMidnightColor());

    QRectF rect(1*G_SCALE, 1*G_SCALE, (WIDTH - 2)*G_SCALE, (HEIGHT - 2)*G_SCALE);
    painter.drawRoundedRect(rect, 10, 100, Qt::RelativeSize);
    painter.end();

    return pixmap;
}

void UpdateAppItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void UpdateAppItem::setMode(IUpdateAppItem::UpdateAppItemMode mode)
{
    if (mode == UPDATE_APP_ITEM_MODE_PROMPT)
    {
        animateTransitionToVersion();
        curProgressBarPos_ = 0;
    }
    else // Progress
    {
        animateTransitionToProgress();
        curProgressBarPos_ = 0;
    }
    mode_ = mode;
}

void UpdateAppItem::onBackgroundOpacityChanged(const QVariant &value)
{
    curBackgroundOpacity_ = value.toDouble();
    update();
}

void UpdateAppItem::onVersionOpacityChanged(const QVariant &value)
{
    curVersionOpacity_ = value.toDouble();
    update();
}

void UpdateAppItem::onProgressForegroundOpacityChanged(const QVariant &value)
{
    curProgressForegroundOpacity_ = value.toDouble();
    update();
}

void UpdateAppItem::onProgressBackgroundOpacityChanged(const QVariant &value)
{
    curProgressBackgroundOpacity_ = value.toDouble();
    update();
}

void UpdateAppItem::onProgressBarPosChanged(const QVariant &value)
{
    curProgressBarPos_ = value.toInt();
    update();
}

void UpdateAppItem::onUpdateClick()
{
    emit updateClick();
}

void UpdateAppItem::onLanguageChanged()
{
    updateButton_->recalcBoundingRect();

    int updatePosX = WIDTH - updateButton_->boundingRect().width() - 15;
    updateButton_->setPos(updatePosX, 0);
    update();
}

void UpdateAppItem::animateTransitionToProgress()
{
    startAnAnimation<double>(versionOpacityAnimation_, curVersionOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    updateButton_->animateHide(ANIMATION_SPEED_FAST);

    startAnAnimation<double>(progressBackgroundOpacityAnimation_, curProgressBackgroundOpacity_, OPACITY_BACKGROUND_PROGRESS, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(progressForegroundOpacityAnimation_, curProgressForegroundOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void UpdateAppItem::animateTransitionToVersion()
{
    startAnAnimation<double>(versionOpacityAnimation_, curVersionOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    updateButton_->animateShow(ANIMATION_SPEED_FAST);

    startAnAnimation<double>(progressBackgroundOpacityAnimation_, curProgressBackgroundOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(progressForegroundOpacityAnimation_, curProgressForegroundOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

void UpdateAppItem::updatePositions()
{
    updateButton_->recalcBoundingRect();
    int updatePosX = WIDTH*G_SCALE - updateButton_->boundingRect().width();
    int updatePosY = (HEIGHT*G_SCALE - updateButton_->boundingRect().height()) / 2 - (G_SCALE+0.5);
    updateButton_->setPos(updatePosX, updatePosY);
}

} // namespace UpdateApp
