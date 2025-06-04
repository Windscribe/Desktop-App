#include "packetsizeeditboxitem.h"

#include <QPainter>
#include <QTimer>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "commongraphics/basepage.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

const int BUSY_SPINNER_OPACITY_ANIMATION_TIME = 300;
const int BUSY_SPINNER_ROTATION_ANIMATION_TIME = 500;
const int ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME = 300;

PacketSizeEditBoxItem::PacketSizeEditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), caption_(caption), detectButton_(NULL), isEditMode_(false),
    detectButtonBusy_(false), busySpinnerOpacity_(OPACITY_HIDDEN), busySpinnerRotation_(0), busySpinnerTimer_(this),
    detectButtonOpacity_(OPACITY_FULL)
{
    editButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/EDIT_ICON", "", this);
    connect(editButton_, &IconButton::clicked, this, &PacketSizeEditBoxItem::onEditClick);

    detectButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/AUTODETECT_ICON", "", this);
    connect(detectButton_, &IconButton::clicked, this, &PacketSizeEditBoxItem::detectButtonClicked);
    connect(detectButton_, &IconButton::hoverEnter, this, &PacketSizeEditBoxItem::detectButtonHoverEnter);
    connect(detectButton_, &IconButton::hoverLeave, this, &PacketSizeEditBoxItem::detectButtonHoverLeave);

    connect(&busySpinnerOpacityAnimation_, &QVariantAnimation::valueChanged, this, &PacketSizeEditBoxItem::onBusySpinnerOpacityAnimationChanged);
    connect(&busySpinnerRotationAnimation_, &QVariantAnimation::valueChanged, this, &PacketSizeEditBoxItem::onBusySpinnerRotationAnimationChanged);
    connect(&busySpinnerRotationAnimation_, &QVariantAnimation::finished, this, &PacketSizeEditBoxItem::onBusySpinnerRotationAnimationFinished);
    connect(&detectButtonOpacityAnimation_, &QVariantAnimation::valueChanged, this, &PacketSizeEditBoxItem::onAdditionalButtonOpacityAnimationValueChanged);
    confirmButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/CONFIRM_ICON", "", this);
    confirmButton_->hide();
    connect(confirmButton_, &IconButton::clicked, this, &PacketSizeEditBoxItem::onConfirmClick);

    busySpinnerTimer_.setSingleShot(true);
    busySpinnerTimer_.setInterval(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME / 3);
    connect(&busySpinnerTimer_, &QTimer::timeout, this, &PacketSizeEditBoxItem::onBusySpinnerStartSpinning);

    undoButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/UNDO_ICON", "", this);
    undoButton_->hide();
    connect(undoButton_, &IconButton::clicked, this, &PacketSizeEditBoxItem::onUndoClick);

    editPlaceholderText_ = editPrompt;

    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(tr(editPrompt.toStdString().c_str()));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);
    proxyWidget_->hide();

    updatePositions();
}

void PacketSizeEditBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!isEditMode_)
    {
        // caption text
        QFont font = FontManager::instance().getFont(12,  QFont::Normal);
        QFontMetrics fm(font);
        QString caption = tr(caption_.toStdString().c_str());
        painter->setFont(font);
        painter->setPen(Qt::white);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  PREFERENCES_ITEM_Y*G_SCALE,
                                                  -(3*PREFERENCES_MARGIN_X + 2*ICON_WIDTH)*G_SCALE,
                                                  -PREFERENCES_MARGIN_Y*G_SCALE),
                          Qt::AlignLeft,
                          caption);

        // user text box
        painter->setOpacity(OPACITY_HALF);
        QString t;
        if (!text_.isEmpty()) {
            t = text_;
        } else {
            t = "--";
        }

        painter->drawText(boundingRect().adjusted((2*PREFERENCES_MARGIN_X*G_SCALE) + fm.horizontalAdvance(caption),
                                                  PREFERENCES_ITEM_Y*G_SCALE,
                                                  -(3*PREFERENCES_MARGIN_X + 2*ICON_WIDTH)*G_SCALE,
                                                  -PREFERENCES_MARGIN_Y*G_SCALE),
                          Qt::AlignRight,
                          fm.elidedText(t,
                                        Qt::ElideRight,
                                        boundingRect().width() - (5*PREFERENCES_MARGIN_X + 2*ICON_WIDTH)*G_SCALE - fm.horizontalAdvance(caption)));

        // spinner
        painter->setOpacity(busySpinnerOpacity_);
        painter->translate(spinnerPosX_ + ICON_WIDTH/2*G_SCALE, spinnerPosY_ + ICON_HEIGHT/2*G_SCALE);
        painter->rotate(busySpinnerRotation_);
        QSharedPointer<IndependentPixmap> spinnerP = ImageResourcesSvg::instance().getIndependentPixmap("SPINNER");
        spinnerP->draw(-ICON_WIDTH/2*G_SCALE, -ICON_HEIGHT/2*G_SCALE, painter);
    }
}

void PacketSizeEditBoxItem::setText(const QString &text)
{
    text_ = text;
    lineEdit_->setText(text_);
    update();
}

void PacketSizeEditBoxItem::setValidator(QRegularExpressionValidator *validator)
{
    lineEdit_->setValidator(validator);
}

void PacketSizeEditBoxItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void PacketSizeEditBoxItem::setEditButtonClickable(bool clickable)
{
    editButton_->setClickable(clickable);
}

void PacketSizeEditBoxItem::onBusySpinnerStartSpinning()
{
    busySpinnerOpacityAnimation_.stop();
    busySpinnerOpacityAnimation_.setDuration(BUSY_SPINNER_OPACITY_ANIMATION_TIME);
    busySpinnerOpacityAnimation_.setStartValue(busySpinnerOpacity_);
    busySpinnerOpacityAnimation_.setEndValue(OPACITY_FULL);
    busySpinnerOpacityAnimation_.start();
}

void PacketSizeEditBoxItem::setDetectButtonBusyState(bool on)
{
    if (detectButtonBusy_ != on) {
        detectButtonBusy_ = on;

        // smooth transition
        if (on) {
            // show spinner
            busySpinnerTimer_.start();

            // start rotation
            busySpinnerRotationAnimation_.stop();
            busySpinnerRotationAnimation_.setDuration(BUSY_SPINNER_ROTATION_ANIMATION_TIME);
            busySpinnerRotationAnimation_.setStartValue(busySpinnerRotation_);
            busySpinnerRotationAnimation_.setEndValue(360);
            busySpinnerRotationAnimation_.start();

            // hide detect button
            detectButtonOpacityAnimation_.stop();
            detectButtonOpacityAnimation_.setDuration(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME);
            detectButtonOpacityAnimation_.setStartValue(detectButtonOpacity_);
            detectButtonOpacityAnimation_.setEndValue(OPACITY_HIDDEN);
            detectButtonOpacityAnimation_.start();
        } else {
            // hide spinner
            busySpinnerTimer_.stop();
            busySpinnerOpacityAnimation_.stop();
            busySpinnerOpacityAnimation_.setDuration(BUSY_SPINNER_OPACITY_ANIMATION_TIME);
            busySpinnerOpacityAnimation_.setStartValue(busySpinnerOpacity_);
            busySpinnerOpacityAnimation_.setEndValue(OPACITY_HIDDEN);
            busySpinnerOpacityAnimation_.start();

            QTimer::singleShot(BUSY_SPINNER_OPACITY_ANIMATION_TIME/2, [this]() {
                // stop rotation
                busySpinnerRotationAnimation_.stop();
                busySpinnerRotation_ = 0;

                // show additional button
                detectButtonOpacityAnimation_.stop();
                detectButtonOpacityAnimation_.setDuration(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME);
                detectButtonOpacityAnimation_.setStartValue(detectButtonOpacity_);
                detectButtonOpacityAnimation_.setEndValue(OPACITY_FULL);
                detectButtonOpacityAnimation_.start();
            });
        }
    }
}

void PacketSizeEditBoxItem::setDetectButtonSelectedState(bool selected)
{
    detectButton_->setSelected(selected);
}

bool PacketSizeEditBoxItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void PacketSizeEditBoxItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        onUndoClick();
        parentItem()->setFocus();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onConfirmClick();
    }
}

void PacketSizeEditBoxItem::onEditClick()
{
    editButton_->hide();
    detectButton_->hide();
    confirmButton_->show();
    undoButton_->show();
    proxyWidget_->show();
    lineEdit_->setFocus();
    isEditMode_ = true;
    update();
}

void PacketSizeEditBoxItem::onConfirmClick()
{
    editButton_->show();
    detectButton_->show();
    confirmButton_->hide();
    undoButton_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    text_ = lineEdit_->text();
    update();
    emit textChanged(text_);
}

void PacketSizeEditBoxItem::onUndoClick()
{
    editButton_->show();
    detectButton_->show();
    confirmButton_->hide();
    undoButton_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    update();
}

void PacketSizeEditBoxItem::onBusySpinnerOpacityAnimationChanged(const QVariant &value)
{
    busySpinnerOpacity_ = value.toDouble();
    update();
}

void PacketSizeEditBoxItem::onBusySpinnerRotationAnimationChanged(const QVariant &value)
{
    busySpinnerRotation_ = value.toInt();
    update();
}

void PacketSizeEditBoxItem::onBusySpinnerRotationAnimationFinished()
{
    // start animation again
    if (detectButtonBusy_)
    {
        busySpinnerRotation_ = 0;
        busySpinnerRotationAnimation_.setDuration(500);
        busySpinnerRotationAnimation_.setStartValue(busySpinnerRotation_);
        busySpinnerRotationAnimation_.setEndValue(360);
        busySpinnerRotationAnimation_.start();
    }
}

void PacketSizeEditBoxItem::onAdditionalButtonOpacityAnimationValueChanged(const QVariant &value)
{
    detectButtonOpacity_ = value.toDouble();
    detectButton_->setOpacity(detectButtonOpacity_);
}

void PacketSizeEditBoxItem::updatePositions()
{
    editButton_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
    detectButton_->setPos(boundingRect().width() - 2*(ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
    confirmButton_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
    undoButton_->setPos(boundingRect().width() - 2*(ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE);
    lineEdit_->setFont(FontManager::instance().getFont(12, QFont::DemiBold));

    spinnerPosX_ = boundingRect().width() - (2*ICON_WIDTH + 2*PREFERENCES_MARGIN_X)*G_SCALE;
    spinnerPosY_ = PREFERENCES_ITEM_Y*G_SCALE;

    if (!proxyWidget_->isVisible()) { // workaround Qt bug (setGeometry not working when proxyWidget_ is not visible)
        proxyWidget_->show();
        lineEdit_->setGeometry(PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE, 180*G_SCALE, ICON_HEIGHT*G_SCALE);
        proxyWidget_->hide();
    } else {
        lineEdit_->setGeometry(PREFERENCES_MARGIN_X*G_SCALE, PREFERENCES_ITEM_Y*G_SCALE, 180*G_SCALE, ICON_HEIGHT*G_SCALE);
    }
}

void PacketSizeEditBoxItem::setCaption(const QString &caption)
{
    caption_ = caption;
    update();
}

void PacketSizeEditBoxItem::setPrompt(const QString &prompt)
{
    lineEdit_->setPlaceholderText(prompt);
}

} // namespace PreferencesWindow
