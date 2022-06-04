#include "packetsizeeditboxitem.h"

#include <QPainter>
#include <QTimer>
#include "../basepage.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

const int BUSY_SPINNER_OPACITY_ANIMATION_TIME = 300;
const int BUSY_SPINNER_ROTATION_ANIMATION_TIME = 500;
const int ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME = 300;

PacketSizeEditBoxItem::PacketSizeEditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt, bool isDrawFullBottomDivider, const QString &additionalButtonIcon) : ScalableGraphicsObject(parent)
  , caption_(caption)
  , btnAdditional_(NULL)
  , isEditMode_(false)
  , additionalButtonBusy_(false)
  , busySpinnerOpacity_(OPACITY_HIDDEN)
  , busySpinnerRotation_(0)
  , busySpinnerTimer_(this)
  , additionalButtonOpacity_(OPACITY_FULL)
{
    line_ = new DividerLine(this, isDrawFullBottomDivider ? 276 : 264);

    btnEdit_ = new IconButton(16, 16, "preferences/EDIT_ICON", "", this);
    connect(btnEdit_, SIGNAL(clicked()), SLOT(onEditClick()));

    if (!additionalButtonIcon.isEmpty())
    {
        btnAdditional_ = new IconButton(16, 16, additionalButtonIcon, "", this);
        connect(btnAdditional_, SIGNAL(clicked()), SIGNAL(additionalButtonClicked()));
        connect(btnAdditional_, SIGNAL(hoverEnter()), SIGNAL(additionalButtonHoverEnter()));
        connect(btnAdditional_, SIGNAL(hoverLeave()), SIGNAL(additionalButtonHoverLeave()));
    }

    connect(&busySpinnerOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBusySpinnerOpacityAnimationChanged(QVariant)));
    connect(&busySpinnerRotationAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBusySpinnerRotationAnimationChanged(QVariant)));
    connect(&busySpinnerRotationAnimation_, SIGNAL(finished()), SLOT(onBusySpinnerRotationAnimationFinished()));
    connect(&additionalButtonOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onAdditionalButtonOpacityAnimationValueChanged(QVariant)));
    btnConfirm_ = new IconButton(16, 16, "preferences/CONFIRM_ICON", "", this);
    btnConfirm_->hide();
    connect(btnConfirm_, SIGNAL(clicked()), SLOT(onConfirmClick()));

    busySpinnerTimer_.setSingleShot(true);
    busySpinnerTimer_.setInterval(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME / 3);
    connect(&busySpinnerTimer_, SIGNAL(timeout()), SLOT(onBusySpinnerStartSpinning()));

    btnUndo_ = new IconButton(16, 16, "preferences/UNDO_ICON", "", this);
    btnUndo_->hide();
    connect(btnUndo_, SIGNAL(clicked()), SLOT(onUndoClick()));

    editPlaceholderText_ = editPrompt;

    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(tr(editPrompt.toStdString().c_str()));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);
    proxyWidget_->hide();

    updatePositions();
}

QRectF PacketSizeEditBoxItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, 43*G_SCALE);
}

void PacketSizeEditBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(QColor(16, 22, 40)));

    if (!isEditMode_)
    {
        // caption text
        QFont *font = FontManager::instance().getFont(12, true);
        painter->setFont(*font);
        painter->setPen(QColor(255, 255, 255));
        painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr(caption_.toStdString().c_str()));

        // user text box
        painter->setOpacity(0.5 * initialOpacity);
        QString t;
        if (!text_.isEmpty())
        {
            t = text_;
        }
        else
        {
            t = "--";
        }
        int rightOffs = -48*G_SCALE;
        if (btnAdditional_)
        {
            rightOffs -= 32*G_SCALE;
        }
        painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, rightOffs, -3*G_SCALE), Qt::AlignRight | Qt::AlignVCenter, t);

        // spinner
        painter->setOpacity(busySpinnerOpacity_ * initialOpacity);
        painter->translate(spinnerPosX_ + 8*G_SCALE, spinnerPosY_ +8*G_SCALE);
        painter->rotate(busySpinnerRotation_);
        QSharedPointer<IndependentPixmap> spinnerP = ImageResourcesSvg::instance().getIndependentPixmap("login/SPINNER");
        spinnerP->draw(-8*G_SCALE, -8*G_SCALE, painter);
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
    updatePositions();
}

void PacketSizeEditBoxItem::setEditButtonClickable(bool clickable)
{
    btnEdit_->setClickable(clickable);
}

void PacketSizeEditBoxItem::onBusySpinnerStartSpinning()
{
    busySpinnerOpacityAnimation_.stop();
    busySpinnerOpacityAnimation_.setDuration(BUSY_SPINNER_OPACITY_ANIMATION_TIME);
    busySpinnerOpacityAnimation_.setStartValue(busySpinnerOpacity_);
    busySpinnerOpacityAnimation_.setEndValue(OPACITY_FULL);
    busySpinnerOpacityAnimation_.start();
}

void PacketSizeEditBoxItem::setAdditionalButtonBusyState(bool on)
{
    if (additionalButtonBusy_ != on)
    {
        additionalButtonBusy_ = on;

        // smooth transition
        if (on)
        {
            // show spinner
            busySpinnerTimer_.start();

            // start rotation
            busySpinnerRotationAnimation_.stop();
            busySpinnerRotationAnimation_.setDuration(BUSY_SPINNER_ROTATION_ANIMATION_TIME);
            busySpinnerRotationAnimation_.setStartValue(busySpinnerRotation_);
            busySpinnerRotationAnimation_.setEndValue(360);
            busySpinnerRotationAnimation_.start();

            // hide additional button
            additionalButtonOpacityAnimation_.stop();
            additionalButtonOpacityAnimation_.setDuration(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME);
            additionalButtonOpacityAnimation_.setStartValue(additionalButtonOpacity_);
            additionalButtonOpacityAnimation_.setEndValue(OPACITY_HIDDEN);
            additionalButtonOpacityAnimation_.start();
        }
        else
        {
            // hide spinner
            busySpinnerTimer_.stop();
            busySpinnerOpacityAnimation_.stop();
            busySpinnerOpacityAnimation_.setDuration(BUSY_SPINNER_OPACITY_ANIMATION_TIME);
            busySpinnerOpacityAnimation_.setStartValue(busySpinnerOpacity_);
            busySpinnerOpacityAnimation_.setEndValue(OPACITY_HIDDEN);
            busySpinnerOpacityAnimation_.start();

            QTimer::singleShot(BUSY_SPINNER_OPACITY_ANIMATION_TIME/2, [this](){
                // stop rotation
                busySpinnerRotationAnimation_.stop();
                busySpinnerRotation_ = 0;

                // show additional button
                additionalButtonOpacityAnimation_.stop();
                additionalButtonOpacityAnimation_.setDuration(ADDITIONAL_BUTTON_OPACITY_ANIMATION_TIME);
                additionalButtonOpacityAnimation_.setStartValue(additionalButtonOpacity_);
                additionalButtonOpacityAnimation_.setEndValue(OPACITY_FULL);
                additionalButtonOpacityAnimation_.start();
            });
        }
    }
}

void PacketSizeEditBoxItem::setAdditionalButtonSelectedState(bool selected)
{
    if (btnAdditional_)
        btnAdditional_->setSelected(selected);
}

bool PacketSizeEditBoxItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void PacketSizeEditBoxItem::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        onUndoClick();
        parentItem()->setFocus();
    }
}

void PacketSizeEditBoxItem::onEditClick()
{
    btnEdit_->hide();
    if (btnAdditional_)
    {
        btnAdditional_->hide();
    }
    btnConfirm_->show();
    btnUndo_->show();
    proxyWidget_->show();
    lineEdit_->setFocus();
    isEditMode_ = true;
    update();
}

void PacketSizeEditBoxItem::onConfirmClick()
{
    btnEdit_->show();
    if (btnAdditional_)
    {
        btnAdditional_->show();
    }

    btnConfirm_->hide();
    btnUndo_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    text_ = lineEdit_->text();
    update();
    emit textChanged(text_);
}

void PacketSizeEditBoxItem::onUndoClick()
{
    btnEdit_->show();
    if (btnAdditional_)
    {
        btnAdditional_->show();
    }
    btnConfirm_->hide();
    btnUndo_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    update();
}

void PacketSizeEditBoxItem::onLanguageChanged()
{
    lineEdit_->setPlaceholderText(tr(editPlaceholderText_.toStdString().c_str()));
}

void PacketSizeEditBoxItem::onBusySpinnerOpacityAnimationChanged(const QVariant &value)
{
    busySpinnerOpacity_ = value.toDouble();
    // btnAdditional_->setOpacity(1 - busySpinnerOpacity_);
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
    if (additionalButtonBusy_)
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
    additionalButtonOpacity_ = value.toDouble();
    btnAdditional_->setOpacity(additionalButtonOpacity_);
}

void PacketSizeEditBoxItem::updatePositions()
{
    line_->setPos(24*G_SCALE, 40*G_SCALE);
    btnEdit_->setPos(boundingRect().width() - btnEdit_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnEdit_->boundingRect().height()) / 2);

    if(btnAdditional_)
    {
        btnAdditional_->setPos(boundingRect().width() - btnEdit_->boundingRect().width() - 16*G_SCALE - btnAdditional_->boundingRect().width() - 16*G_SCALE,
                               (boundingRect().height() - line_->boundingRect().height() - btnAdditional_->boundingRect().height()) / 2);
    }
    btnConfirm_->setPos(boundingRect().width() - btnConfirm_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnConfirm_->boundingRect().height()) / 2);
    btnUndo_->setPos(boundingRect().width() - btnUndo_->boundingRect().width() - (16 + 32)*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnUndo_->boundingRect().height()) / 2);
    lineEdit_->setFont(*FontManager::instance().getFont(12, true));

    spinnerPosX_ = boundingRect().width() - btnEdit_->boundingRect().width() - 16*G_SCALE*3;
    spinnerPosY_ = (boundingRect().height() - line_->boundingRect().height() - 16*G_SCALE) / 2;

    if (!proxyWidget_->isVisible()) // workaround Qt bug (setGeometry now working when proxyWidget_ is not visible)
    {
        proxyWidget_->show();
        lineEdit_->setGeometry((24 + 14)*G_SCALE, 0, 180 * G_SCALE, 40 * G_SCALE);
        proxyWidget_->hide();
    }
    else
    {
        lineEdit_->setGeometry((24 + 14)*G_SCALE, 0, 180 * G_SCALE, 40 * G_SCALE);
    }
}

} // namespace PreferencesWindow

