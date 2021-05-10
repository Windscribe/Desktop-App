#include "updatewindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace UpdateWindow {


UpdateWindowItem::UpdateWindowItem(ScalableGraphicsObject *parent) :
    ScalableGraphicsObject(parent),
    downloading_(false)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    curTitleOpacity_            = OPACITY_FULL;
    curDescriptionOpacity_      = OPACITY_FULL;

    curLowerTitleOpacity_       = OPACITY_HIDDEN;
    curLowerDescriptionOpacity_ = OPACITY_HIDDEN;
    spinnerOpacity_ = OPACITY_HIDDEN;

    curProgress_ = 0;
    curVersion_ = "2.0";
    curTitleText_ = QString("v") + curVersion_;

    connect(&titleOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onTitleOpacityChange(QVariant)));
    connect(&lowerTitleOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onLowerTitleOpacityChange(QVariant)));
    connect(&descriptionOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onDescriptionOpacityChange(QVariant)));
    connect(&lowerDescriptionOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onLowerDescriptionOpacityChange(QVariant)));
    connect(&spinnerOpacityAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onSpinnerOpacityChange(QVariant)));
    connect(&spinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), this, SLOT(onSpinnerRotationChange(QVariant)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    // accept
    acceptButton_ = new CommonGraphics::BubbleButtonBright(this, 128, 40, 20, 20);
    connect(acceptButton_, SIGNAL(clicked()), this, SLOT(onAcceptClick()));
    QString updateText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonBright", "Update");
    acceptButton_->setText(updateText);

    // cancel
    QString cancelText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "Later");
    double cancelOpacity = OPACITY_UNHOVER_TEXT;
    cancelButton_ = new CommonGraphics::TextButton(cancelText, FontDescr(16, false),
                                                   Qt::white, true, this);
    connect(cancelButton_, SIGNAL(clicked()), this, SLOT(onCancelClick()));
    cancelButton_->quickSetOpacity(cancelOpacity);

    updatePositions();
}

QRectF UpdateWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH * G_SCALE, WINDOW_HEIGHT * G_SCALE);
}

void UpdateWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background:
#ifdef Q_OS_WIN
    const QString background = "background/WIN_MAIN_BG";
#else
    const QString background = "background/MAC_MAIN_BG";
#endif
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(background);
    p->draw(0, 0, painter);

    // Text:

    // title
    painter->setOpacity(curTitleOpacity_ * initialOpacity);
    painter->setPen(FontManager::instance().getBrightYellowColor());

    QFont titleFont = *FontManager::instance().getFont(24, true);
    painter->setFont(titleFont);

    QRectF titleRect(0, TITLE_POS_Y * G_SCALE, WINDOW_WIDTH * G_SCALE, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr(curTitleText_.toStdString().c_str()));

    painter->setOpacity(curLowerTitleOpacity_ * initialOpacity);
    painter->setFont(*FontManager::instance().getFont(24, false));
    QRectF lowerTitleRect(0, TITLE_POS_Y * G_SCALE, WINDOW_WIDTH * G_SCALE, CommonGraphics::textHeight(titleFont));

    const QString lowerTitleText = tr("Updating ");
    int widthUpdating = CommonGraphics::textWidth(lowerTitleText, *FontManager::instance().getFont(24, false));

    QString progressPercent = QString("%1%").arg(curProgress_);
    int widthPercent = CommonGraphics::textWidth(progressPercent, *FontManager::instance().getFont(24, false));

    int widthTotal = widthUpdating + widthPercent;
    int posX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, widthTotal);
    painter->drawText(posX, TITLE_POS_Y * G_SCALE + CommonGraphics::textHeight(titleFont),lowerTitleText);

    painter->setFont(titleFont);
    painter->drawText(posX + widthUpdating, TITLE_POS_Y * G_SCALE + CommonGraphics::textHeight(titleFont), progressPercent);

    // main description
    painter->setOpacity(curDescriptionOpacity_ * initialOpacity);
    QFont descFont(*FontManager::instance().getFont(14, false));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    const QString descText = tr("You're about to update Windscribe. "
                                "The application will terminate the active connection and restart automatically.");
    int width = CommonGraphics::idealWidthOfTextRect(DESCRIPTION_WIDTH_MIN * G_SCALE, WINDOW_WIDTH * G_SCALE, 3,
                                                     descText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, width), DESCRIPTION_POS_Y * G_SCALE,
                      width, WINDOW_HEIGHT * G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descText);


    // lower description
    painter->setOpacity(curLowerDescriptionOpacity_ * initialOpacity);
    const QString lowerDescText = tr("Your update is in progress, hang in there...");
    int lowerDescWidth = CommonGraphics::idealWidthOfTextRect(LOWER_DESCRIPTION_WIDTH_MIN * G_SCALE,
                                                              WINDOW_WIDTH * G_SCALE, 2,
                                                              lowerDescText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, lowerDescWidth), LOWER_DESCRIPTION_POS_Y * G_SCALE,
                      lowerDescWidth, WINDOW_HEIGHT * G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, lowerDescText);

	// spinner
    painter->setOpacity(spinnerOpacity_ * initialOpacity);
    const int spinnerPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, SPINNER_HALF_WIDTH * 2 * G_SCALE);
    painter->translate(spinnerPosX+SPINNER_HALF_WIDTH * G_SCALE, SPINNER_POS_Y * G_SCALE+SPINNER_HALF_HEIGHT * G_SCALE);
    painter->rotate(spinnerRotation_);
    IndependentPixmap *pSpinner = ImageResourcesSvg::instance().getIndependentPixmap("update/UPDATING_SPINNER");
    pSpinner->draw(-SPINNER_HALF_WIDTH * G_SCALE,-SPINNER_HALF_HEIGHT * G_SCALE, painter);
}

void UpdateWindowItem::setClickable(bool isClickable)
{
    acceptButton_->setClickable(isClickable);
    cancelButton_->setClickable(isClickable);
}

void UpdateWindowItem::setVersion(QString version, int buildNumber)
{
    curVersion_ = version;
    curTitleText_ = QString("v") + curVersion_+ "." + QString::number(buildNumber);
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
    spinnerRotation_ = 0;
    startAnAnimation(spinnerRotationAnimation_, spinnerRotation_, spinnerRotation_ + SPINNER_ROTATION_TARGET, ANIMATION_SPEED_VERY_SLOW);
    QTimer::singleShot(5000, this, SLOT(onUpdatingTimeout()));
}

void UpdateWindowItem::stopAnimation()
{
    spinnerRotationAnimation_.stop();
}

void UpdateWindowItem::updateScaling()
{
    updatePositions();
}

void UpdateWindowItem::changeToDownloadingScreen()
{
    downloading_ = true;
    cancelButton_->setText(cancelButtonText());

    int animationSpeed = ANIMATION_SPEED_SLOW;

    // hide first screen
    startAnAnimation(titleOpacityAnimation_, curTitleOpacity_, OPACITY_HIDDEN, animationSpeed);
    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_HIDDEN, animationSpeed);

    acceptButton_->animateHide(animationSpeed);
    // cancelButton_->animateHide(animationSpeed); // TODO: uncomment later -- timer will make "cancel" reappear

    // show Updating screen
    startAnAnimation(lowerTitleOpacityAnimation_, curLowerTitleOpacity_, OPACITY_FULL, animationSpeed);
    startAnAnimation(lowerDescriptionOpacityAnimation_, curLowerDescriptionOpacity_, OPACITY_FULL, animationSpeed);
    startAnAnimation(spinnerOpacityAnimation_, spinnerOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, animationSpeed);
}

void UpdateWindowItem::changeToPromptScreen()
{
    downloading_ = false;
    cancelButton_->setText(cancelButtonText());

    int animationSpeed = ANIMATION_SPEED_SLOW;

    // hide downloading screen
    startAnAnimation(lowerTitleOpacityAnimation_, curLowerTitleOpacity_, OPACITY_HIDDEN, animationSpeed);
    startAnAnimation(lowerDescriptionOpacityAnimation_, curLowerDescriptionOpacity_, OPACITY_HIDDEN, animationSpeed);
    startAnAnimation(spinnerOpacityAnimation_, spinnerOpacity_, OPACITY_HIDDEN, animationSpeed);

    acceptButton_->animateShow(animationSpeed);
    cancelButton_->animateShow(animationSpeed);

    // show prompt screen
    startAnAnimation(titleOpacityAnimation_, curTitleOpacity_, OPACITY_FULL, animationSpeed);
    startAnAnimation(descriptionOpacityAnimation_, curDescriptionOpacity_, OPACITY_FULL, animationSpeed);
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

const QString UpdateWindowItem::cancelButtonText()
{
    QString text = tr("Cancel");
    if (!downloading_) text = tr("Later");
    return text;
}

void UpdateWindowItem::onAcceptClick()
{
    changeToDownloadingScreen();
    emit acceptClick();
}

void UpdateWindowItem::onCancelClick()
{
    if (downloading_)
    {
        emit cancelClick();
    }
    else
    {
        emit laterClick();
    }
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
    cancelButton_->setText(cancelButtonText());
    cancelButton_->recalcBoundingRect();

    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y * G_SCALE);
}

void UpdateWindowItem::updatePositions()
{
    acceptButton_->updateScaling();
    int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, ACCEPT_BUTTON_POS_Y * G_SCALE);

    cancelButton_->updateScaling();
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y * G_SCALE);
}


} // namespace
