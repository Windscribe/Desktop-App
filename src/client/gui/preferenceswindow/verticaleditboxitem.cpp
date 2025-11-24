#include "verticaleditboxitem.h"

#include <QPainter>
#include <QStyleOption>
#include "commongraphics/basepage.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

VerticalEditBoxItem::VerticalEditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), caption_(caption), isEditMode_(false), maskingChar_('\0')
{
    btnEdit_ = new IconButton(16, 16, "EDIT_ICON", "", this);
    connect(btnEdit_, &IconButton::clicked, this, &VerticalEditBoxItem::onEditClick);

    btnConfirm_ = new IconButton(16, 16, "preferences/CONFIRM_ICON", "", this);
    btnConfirm_->hide();
    connect(btnConfirm_, &IconButton::clicked, this, &VerticalEditBoxItem::onConfirmClick);

    btnUndo_ = new IconButton(16, 16, "preferences/UNDO_ICON", "", this);
    btnUndo_->hide();
    connect(btnUndo_, &IconButton::clicked, this, &VerticalEditBoxItem::onUndoClick);

    editPlaceholderText_ = editPrompt;

    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(tr(editPrompt.toStdString().c_str()));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);
    proxyWidget_->hide();

    updateScaling();
}

void VerticalEditBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QFont font = FontManager::instance().getFont(12,  QFont::Normal);
    QFontMetrics fm(font);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                              PREFERENCES_ITEM_Y*G_SCALE,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              -PREFERENCES_MARGIN_Y*G_SCALE),
                      Qt::AlignLeft, caption_);

    if (!isEditMode_) {
        painter->setOpacity(OPACITY_HALF);
        QString t;
        if (text_.isEmpty()) {
            t = "--";
        } else if (!maskingChar_.isNull()) {
            t = QString(text_.size(), maskingChar_);
        } else {
            t = text_;
        }

        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  (PREFERENCES_ITEM_Y + PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE,
                                                  -(2*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE,
                                                  -PREFERENCES_MARGIN_Y),
                          Qt::AlignLeft,
                          fm.elidedText(t,
                                        Qt::ElideRight,
                                        boundingRect().width() - (3*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE));
    }

    // Draw error message if present
    if (!errorText_.isEmpty()) {
        QFont errorFont = FontManager::instance().getFont(10, QFont::Normal);
        painter->setFont(errorFont);
        painter->setPen(Qt::red);
        painter->setOpacity(OPACITY_FULL);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE,
                                                  boundingRect().height() - PREFERENCES_MARGIN_Y*G_SCALE - errorHeight_,
                                                  -PREFERENCES_MARGIN_X*G_SCALE,
                                                  0), Qt::AlignLeft | Qt::TextWordWrap, errorText_);
    }
}

void VerticalEditBoxItem::setCaption(const QString &caption)
{
    caption_ = caption;
    update();
}

void VerticalEditBoxItem::setText(const QString &text)
{
    text_ = text;
    lineEdit_->setText(text_);
    update();
}

void VerticalEditBoxItem::setPrompt(const QString &prompt)
{
    editPlaceholderText_ = prompt;
    lineEdit_->setPlaceholderText(editPlaceholderText_.toStdString().c_str());
}

void VerticalEditBoxItem::setValidator(QRegularExpressionValidator *validator)
{
    lineEdit_->setValidator(validator);
}

void VerticalEditBoxItem::setError(const QString &error)
{
    errorText_ = error;
    updatePositions();
}

void VerticalEditBoxItem::setEditButtonClickable(bool clickable)
{
    btnEdit_->setClickable(clickable);
}

void VerticalEditBoxItem::setMasked(bool masked)
{
    if (masked) {
        QStyleOptionFrame opt;
        opt.initFrom(lineEdit_);
        maskingChar_ = QChar(lineEdit_->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, lineEdit_));
        lineEdit_->setEchoMode(QLineEdit::Password);
    } else {
        maskingChar_ = '\0';
        lineEdit_->setEchoMode(QLineEdit::Normal);
    }
    update();
}

bool VerticalEditBoxItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void VerticalEditBoxItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        onConfirmClick();
    }
    else if (event->key() == Qt::Key_Escape)
    {
        onUndoClick();
        parentItem()->setFocus();
    }
}

void VerticalEditBoxItem::onEditClick()
{
    btnEdit_->hide();
    btnConfirm_->show();
    btnUndo_->show();
    proxyWidget_->show();
    lineEdit_->setFocus();
    isEditMode_ = true;
    update();
}

void VerticalEditBoxItem::onConfirmClick()
{
    btnEdit_->show();
    btnConfirm_->hide();
    btnUndo_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    text_ = lineEdit_->text();
    update();
    emit textChanged(text_);
}

void VerticalEditBoxItem::onUndoClick()
{
    btnEdit_->show();
    btnConfirm_->hide();
    btnUndo_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    update();
}

void VerticalEditBoxItem::updateScaling()
{
    CommonGraphics::BaseItem::updateScaling();
    setHeight((PREFERENCE_GROUP_ITEM_HEIGHT + PREFERENCES_ITEM_Y + ICON_HEIGHT)*G_SCALE);
    updatePositions();
}

void VerticalEditBoxItem::updatePositions()
{
    qreal top = (PREFERENCES_ITEM_Y + PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE;
    btnEdit_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, top);
    btnConfirm_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN_X)*G_SCALE, top);
    btnUndo_->setPos(boundingRect().width() - (2*ICON_WIDTH + 2*PREFERENCES_MARGIN_X)*G_SCALE, top);
    lineEdit_->setFont(FontManager::instance().getFont(12,  QFont::Normal));

    if (!proxyWidget_->isVisible()) // workaround Qt bug (setGeometry not working when proxyWidget_ is not visible)
    {
        proxyWidget_->show();
        lineEdit_->setGeometry((PREFERENCES_MARGIN_X)*G_SCALE, top, boundingRect().width() - (2*ICON_WIDTH + 3*PREFERENCES_MARGIN_X)*G_SCALE, ICON_HEIGHT*G_SCALE);
        proxyWidget_->hide();
    }
    else
    {
        lineEdit_->setGeometry((PREFERENCES_MARGIN_X)*G_SCALE, top, boundingRect().width() - (2*ICON_WIDTH + 3*PREFERENCES_MARGIN_X)*G_SCALE, ICON_HEIGHT*G_SCALE);
    }

    // Calculate error message height and adjust widget height
    if (errorText_.isEmpty()) {
        errorHeight_ = 0;
        setHeight((PREFERENCE_GROUP_ITEM_HEIGHT + PREFERENCES_ITEM_Y + ICON_HEIGHT)*G_SCALE);
    } else {
        QFontMetrics fm(FontManager::instance().getFont(10, QFont::Normal));
        errorHeight_ = fm.boundingRect(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0).toRect(),
                                       Qt::AlignLeft | Qt::TextWordWrap,
                                       errorText_).height();
        setHeight((PREFERENCE_GROUP_ITEM_HEIGHT + PREFERENCES_ITEM_Y + ICON_HEIGHT)*G_SCALE + errorHeight_ + DESCRIPTION_MARGIN*G_SCALE);
    }
    update();
}

} // namespace PreferencesWindow

