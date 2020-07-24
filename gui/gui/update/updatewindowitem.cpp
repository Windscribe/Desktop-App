#include "updatewindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"

namespace UpdateWindow {


UpdateWindowItem::UpdateWindowItem(bool upgrade, ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    curBackgroundOpacity_       = OPACITY_FULL;
    curTitleOpacity_            = OPACITY_FULL;
    curDescriptionOpacity_      = OPACITY_FULL;

    curLowerTitleOpacity_       = OPACITY_HIDDEN;
    curLowerDescriptionOpacity_ = OPACITY_HIDDEN;
    spinnerOpacity_ = OPACITY_HIDDEN;

    curVersion_ = "2.0";
    curProgress_ = 0;

#ifdef Q_OS_WIN
    background_ = "background/WIN_MAIN_BG";
#else
    background_ = "background/MAC_MAIN_BG";
#endif

    connect(&titleOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onTitleOpacityChange(QVariant)));
    connect(&lowerTitleOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onLowerTitleOpacityChange(QVariant)));
    connect(&descriptionOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onDescriptionOpacityChange(QVariant)));
    connect(&lowerDescriptionOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onLowerDescriptionOpacityChange(QVariant)));

    connect(&spinnerOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onSpinnerOpacityChange(QVariant)));
    connect(&spinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onSpinnerRotationChange(QVariant)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    acceptButton_ = new CommonGraphics::BubbleButtonBright(this, 128, 40, 20, 20);
    int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, ACCEPT_BUTTON_POS_Y);
    connect(acceptButton_, SIGNAL(clicked()), this, SLOT(onAcceptClick()));

    upgradeMode_ = upgrade;

    QString cancelText;
    double cancelOpacity;
    if (upgrade)
    {
        curTitleText_ = QT_TR_NOOP("You're out of data!");
        titleColor_ = FontManager::instance().getErrorRedColor();
        curDescriptionText_ = QT_TR_NOOP("Don't leave your front door open. "
                                 "Upgrade or wait until next month to get your monthly data allowance back.");
        curLowerDescriptionText_ = QT_TR_NOOP("");

        QString upgradeText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonBright", "Upgrade");
        acceptButton_->setText(upgradeText);

        cancelText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "I'm broke");
        cancelOpacity = OPACITY_UNHOVER_TEXT;
    }
    else
    {
        curTitleText_ = QString("v") + curVersion_;
        curDescriptionText_ = QT_TR_NOOP("You're about to update Windscribe. "
                                 "The application will auto-restart after it is complete.");

        titleColor_ = FontManager::instance().getBrightYellowColor();

        curLowerTitleText_ = QT_TR_NOOP("Updating ");
        curLowerDescriptionText_ = QT_TR_NOOP("Your update is in progress, hang in there...");

        QString updateText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonBright", "Update");
        acceptButton_->setText(updateText);

        cancelText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Cancel");
        cancelOpacity = OPACITY_UNHOVER_TEXT;
    }

    cancelButton_ = new CommonGraphics::TextButton(cancelText, FontDescr(16, false),
                                                   Qt::white, true, this);
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y);
    connect(cancelButton_, SIGNAL(clicked()), this, SLOT(onCancelClick()));
    cancelButton_->quickSetOpacity(cancelOpacity);

}

QRectF UpdateWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void UpdateWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background:
    painter->setOpacity(curBackgroundOpacity_ * initialOpacity);

    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(background_);
    p->draw(0, 0, painter);

    // Text:

    // title
    painter->setOpacity(curTitleOpacity_ * initialOpacity);
    painter->setPen(titleColor_);

    QFont titleFont = *FontManager::instance().getFont(24, true);
    painter->setFont(titleFont);

    QRectF titleRect(0, TITLE_POS_Y, WINDOW_WIDTH, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr(curTitleText_.toStdString().c_str()));

    painter->setOpacity(curLowerTitleOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(24, false));
    QRectF lowerTitleRect(0, TITLE_POS_Y, WINDOW_WIDTH, CommonGraphics::textHeight(titleFont));

    int widthUpdating = CommonGraphics::textWidth(tr(curLowerTitleText_.toStdString().c_str()), *FontManager::instance().getFont(24, false));

    QString progressPercent = QString("%1%").arg(curProgress_);
    int widthPercent = CommonGraphics::textWidth(progressPercent, *FontManager::instance().getFont(24, false));

    int widthTotal = widthUpdating + widthPercent;
    int posX = CommonGraphics::centeredOffset(WINDOW_WIDTH, widthTotal);

    painter->drawText(posX, TITLE_POS_Y + CommonGraphics::textHeight(titleFont), tr(curLowerTitleText_.toStdString().c_str()));

    painter->setFont(titleFont);
    painter->drawText(posX + widthUpdating, TITLE_POS_Y + CommonGraphics::textHeight(titleFont), progressPercent);

    // main description
    painter->setOpacity(curDescriptionOpacity_ * initialOpacity);
    QFont descFont(*FontManager::instance().getFont(14, false));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    int width = CommonGraphics::idealWidthOfTextRect(DESCRIPTION_WIDTH_MIN, WINDOW_WIDTH, 3, tr(curDescriptionText_.toStdString().c_str()), descFont);

    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH, width), DESCRIPTION_POS_Y,
                      width, WINDOW_HEIGHT,
                      Qt::AlignHCenter | Qt::TextWordWrap, tr(curDescriptionText_.toStdString().c_str()));


    // lower description
    painter->setOpacity(curLowerDescriptionOpacity_ * initialOpacity);
    int lowerDescWidth = CommonGraphics::idealWidthOfTextRect(LOWER_DESCRIPTION_WIDTH_MIN, WINDOW_WIDTH, 2, tr(curLowerDescriptionText_.toStdString().c_str()), descFont);

    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH, lowerDescWidth), LOWER_DESCRIPTION_POS_Y,
                      lowerDescWidth, WINDOW_HEIGHT,
                      Qt::AlignHCenter | Qt::TextWordWrap, tr(curLowerDescriptionText_.toStdString().c_str()));

	// spinner
    painter->setOpacity(spinnerOpacity_ * initialOpacity);
    const int spinnerPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH, SPINNER_HALF_WIDTH*2);
    painter->translate(spinnerPosX+SPINNER_HALF_WIDTH, SPINNER_POS_Y+SPINNER_HALF_HEIGHT);
    painter->rotate(spinnerRotation_);
    IndependentPixmap *pSpinner = ImageResourcesSvg::instance().getIndependentPixmap("update/UPDATING_SPINNER");
    pSpinner->draw(-SPINNER_HALF_WIDTH,-SPINNER_HALF_HEIGHT, painter);

}

void UpdateWindowItem::setVersion(QString version)
{
    curVersion_ = version;

    curTitleText_ = QString("v") + curVersion_;

    initScreen();
}

void UpdateWindowItem::setProgress(int progressPercent)
{
    if (progressPercent < 0) progressPercent = 0;
    else if (progressPercent > 100) progressPercent = 100;

    curProgress_ = progressPercent;
    update();

}

void UpdateWindowItem::startAnimation()
{
    if (!upgradeMode_)
    {
        spinnerRotation_ = 0;
        startAnAnimation(spinnerRotationAnimation_, spinnerRotation_, spinnerRotation_ + SPINNER_ROTATION_TARGET, ANIMATION_SPEED_VERY_SLOW);

        QTimer::singleShot(5000, this, SLOT(onUpdatingTimeout()));
    }
}

void UpdateWindowItem::stopAnimation()
{
    spinnerRotationAnimation_.stop();
}

void UpdateWindowItem::changeToDownloadingScreen()
{
    if (!upgradeMode_)
    {
        int animationSpeed = ANIMATION_SPEED_SLOW;

        // hide first screen
        startAnAnimation(titleOpacityAnimation_, curTitleOpacity_, OPACITY_HIDDEN, animationSpeed);
        startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_HIDDEN, animationSpeed);

        acceptButton_->animateHide(animationSpeed);
        cancelButton_->animateHide(animationSpeed);

        // show Updating screen
        startAnAnimation(lowerTitleOpacityAnimation_, curLowerTitleOpacity_, OPACITY_FULL, animationSpeed);
        startAnAnimation(lowerDescriptionOpacityAnimation_, curLowerDescriptionOpacity_, OPACITY_FULL, animationSpeed);
        startAnAnimation(spinnerOpacityAnimation_, spinnerOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, animationSpeed);
    }
}

void UpdateWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit cancelClick();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        onAcceptClick();
    }
}

void UpdateWindowItem::initScreen()
{
    curTitleOpacity_ = OPACITY_FULL;
    curDescriptionOpacity_ = OPACITY_FULL;

    acceptButton_->animateShow(ANIMATION_SPEED_VERY_FAST);
    cancelButton_->animateShow(ANIMATION_SPEED_VERY_FAST);

    curLowerTitleOpacity_ = OPACITY_HIDDEN;
    curLowerDescriptionOpacity_ = OPACITY_HIDDEN;
    spinnerOpacity_ = OPACITY_HIDDEN;
}

void UpdateWindowItem::onAcceptClick()
{
    if (!upgradeMode_)
    {
        changeToDownloadingScreen();
    }

    emit acceptClick();
}

void UpdateWindowItem::onCancelClick()
{
    emit cancelClick();
}

void UpdateWindowItem::onBackgroundOpacityChange(const QVariant &value)
{
    curBackgroundOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onTitleOpacityChange(const QVariant &value)
{
    curTitleOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onLowerTitleOpacityChange(const QVariant &value)
{
    curLowerTitleOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onDescriptionOpacityChange(const QVariant &value)
{
    curDescriptionOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onLowerDescriptionOpacityChange(const QVariant &value)
{
    curLowerDescriptionOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onSpinnerOpacityChange(const QVariant &value)
{
    spinnerOpacity_ = value.toDouble();
    update();
}

void UpdateWindowItem::onSpinnerRotationChange(const QVariant &value)
{
    spinnerRotation_ = value.toInt();

    // keep alive
    if (abs(spinnerRotation_ - SPINNER_ROTATION_TARGET) < 5)
    {
        startAnimation();
    }

    update();
}

void UpdateWindowItem::onUpdatingTimeout()
{
    if (cancelButton_->getOpacity() < OPACITY_UNHOVER_TEXT)
    {
        cancelButton_->animateShow(ANIMATION_SPEED_SLOW);
    }
}

void UpdateWindowItem::onLanguageChanged()
{
    cancelButton_->recalcBoundingRect();

    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y);
}


} // namespace
