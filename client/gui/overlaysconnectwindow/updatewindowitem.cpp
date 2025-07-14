#include "updatewindowitem.h"

#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

#ifdef Q_OS_LINUX
#include "utils/linuxutils.h"
#endif

UpdateWindowItem::UpdateWindowItem(Preferences *preferences, ScalableGraphicsObject *parent) :
    ScalableGraphicsObject(parent), preferences_(preferences), downloading_(false), height_(WINDOW_HEIGHT)
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

    connect(preferences_, &Preferences::appSkinChanged, this, &UpdateWindowItem::onAppSkinChanged);

    connect(&titleOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onTitleOpacityChange);
    connect(&lowerTitleOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onLowerTitleOpacityChange);
    connect(&descriptionOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onDescriptionOpacityChange);
    connect(&lowerDescriptionOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onLowerDescriptionOpacityChange);
    connect(&spinnerOpacityAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onSpinnerOpacityChange);
    connect(&spinnerRotationAnimation_, &QVariantAnimation::valueChanged, this, &UpdateWindowItem::onSpinnerRotationChange);

    // accept
    acceptButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kBright, 128, 40, 20);
    connect(acceptButton_, &CommonGraphics::BubbleButton::clicked, this, &UpdateWindowItem::onAcceptClick);

    // cancel
    double cancelOpacity = OPACITY_UNHOVER_TEXT;
    cancelButton_ = new CommonGraphics::TextButton("", FontDescr(16, QFont::Normal), Qt::white, true, this);
    connect(cancelButton_, &CommonGraphics::TextButton::clicked, this, &UpdateWindowItem::onCancelClick);
    cancelButton_->quickSetOpacity(cancelOpacity);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &UpdateWindowItem::onLanguageChanged);
    onLanguageChanged();

    updatePositions();
}

QRectF UpdateWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, height_*G_SCALE);
}

void UpdateWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        painter->setPen(Qt::NoPen);
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
    } else {
        const QString background = "background/MAIN_BG";
        painter->setOpacity(OPACITY_FULL * initialOpacity);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(background);
        p->draw(0, 0, painter);

        QString borderInner = "background/MAIN_BORDER_INNER";
        QSharedPointer<IndependentPixmap> borderInnerPixmap = ImageResourcesSvg::instance().getIndependentPixmap(borderInner);
        borderInnerPixmap->draw(0, 0, WINDOW_WIDTH*G_SCALE, (WINDOW_HEIGHT-1)*G_SCALE, painter);
    }

    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -16*G_SCALE : 0;

    // title
    painter->setOpacity(curTitleOpacity_ * initialOpacity);
    painter->setPen(FontManager::instance().getBrightYellowColor());

    QFont titleFont = FontManager::instance().getFont(24, QFont::Bold);
    painter->setFont(titleFont);

    QRectF titleRect(0, (TITLE_POS_Y + yOffset)*G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr(curTitleText_.toStdString().c_str()));

    painter->setOpacity(curLowerTitleOpacity_ * initialOpacity);
    painter->setFont(FontManager::instance().getFont(24,  QFont::Normal));
    QRectF lowerTitleRect(0, (TITLE_POS_Y + yOffset)*G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));

    const QString lowerTitleText = tr("Updating ");
    int widthUpdating = CommonGraphics::textWidth(lowerTitleText, FontManager::instance().getFont(24,  QFont::Normal));

    QString progressPercent = QString("%1%").arg(curProgress_);
    int widthPercent = CommonGraphics::textWidth(progressPercent, FontManager::instance().getFont(24,  QFont::Normal));

    int widthTotal = widthUpdating + widthPercent;
    int posX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, widthTotal);
    painter->drawText(posX, (TITLE_POS_Y + yOffset)*G_SCALE + CommonGraphics::textHeight(titleFont),lowerTitleText);

    painter->setFont(titleFont);
    painter->drawText(posX + widthUpdating, (TITLE_POS_Y + yOffset)*G_SCALE + CommonGraphics::textHeight(titleFont), progressPercent);

    // main description
    painter->setOpacity(curDescriptionOpacity_ * initialOpacity);
    QFont descFont(FontManager::instance().getFont(14,  QFont::Normal));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    QString descText;
#ifdef Q_OS_LINUX
    if (LinuxUtils::isImmutableDistro()) {
        descText = tr("Windscribe will download and install the update, which may take several minutes. Your computer will restart after the update.");
    } else {
#else
    {
#endif
        descText = tr("Windscribe will download the update, then terminate active connections and restart automatically.");
    }
    int width = CommonGraphics::idealWidthOfTextRect(DESCRIPTION_WIDTH_MIN*G_SCALE, WINDOW_WIDTH*G_SCALE, 3,
                                                     descText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, width), (DESCRIPTION_POS_Y + yOffset)*G_SCALE,
                      width, height_*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descText);

    // lower description
    painter->setOpacity(curLowerDescriptionOpacity_ * initialOpacity);
    const QString lowerDescText = tr("Your update is in progress, hang in there...");
    int lowerDescWidth = CommonGraphics::idealWidthOfTextRect(LOWER_DESCRIPTION_WIDTH_MIN*G_SCALE, WINDOW_WIDTH*G_SCALE, 2,
                                                              lowerDescText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, lowerDescWidth),
                      (LOWER_DESCRIPTION_POS_Y + yOffset)*G_SCALE,
                      lowerDescWidth, height_*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, lowerDescText);

    // spinner
    painter->setOpacity(spinnerOpacity_ * initialOpacity);
    const int spinnerPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, SPINNER_HALF_WIDTH*2*G_SCALE);
    painter->translate(spinnerPosX + SPINNER_HALF_WIDTH*G_SCALE, (SPINNER_POS_Y + yOffset)*G_SCALE + SPINNER_HALF_HEIGHT*G_SCALE);
    painter->rotate(spinnerRotation_);
    QSharedPointer<IndependentPixmap> pSpinner = ImageResourcesSvg::instance().getIndependentPixmap("update/UPDATING_SPINNER");
    pSpinner->draw(-SPINNER_HALF_WIDTH*G_SCALE, -SPINNER_HALF_HEIGHT*G_SCALE, painter);
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
    updatePositions();

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
    updatePositions();

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

    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, cancelButton_->boundingRect().width());
    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -28*G_SCALE : 0;
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y*G_SCALE + yOffset);

    acceptButton_->setText(tr("Update"));
    cancelButton_->setText(tr("Later"));
}

void UpdateWindowItem::updatePositions()
{
    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -28*G_SCALE : 0;

    acceptButton_->updateScaling();
    int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, ACCEPT_BUTTON_POS_Y*G_SCALE + yOffset);

    cancelButton_->updateScaling();
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, cancelButton_->boundingRect().width());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y*G_SCALE + yOffset);
}

void UpdateWindowItem::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    update();
}

void UpdateWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    height_ = height;
    updatePositions();
}
