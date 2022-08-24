#include "editboxitem.h"

#include <QPainter>
#include <QStyleOption>
#include "commongraphics/basepage.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

EditBoxItem::EditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), caption_(caption), isEditMode_(false), maskingChar_('\0')
{
    btnEdit_ = new IconButton(16, 16, "preferences/EDIT_ICON", "", this);
    connect(btnEdit_, &IconButton::clicked, this, &EditBoxItem::onEditClick);

    btnConfirm_ = new IconButton(16, 16, "preferences/CONFIRM_ICON", "", this);
    btnConfirm_->hide();
    connect(btnConfirm_, &IconButton::clicked, this, &EditBoxItem::onConfirmClick);

    btnUndo_ = new IconButton(16, 16, "preferences/UNDO_ICON", "", this);
    btnUndo_->hide();
    connect(btnUndo_, &IconButton::clicked, this, &EditBoxItem::onUndoClick);

    editPlaceholderText_ = editPrompt;

    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(tr(editPrompt.toStdString().c_str()));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &EditBoxItem::onLanguageChanged);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);
    proxyWidget_->hide();

    updateScaling();
}

void EditBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!isEditMode_)
    {
        QFont *font = FontManager::instance().getFont(12, false);
        painter->setFont(*font);
        painter->setPen(Qt::white);
        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                                  PREFERENCES_MARGIN*G_SCALE,
                                                  -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                                  -PREFERENCES_MARGIN*G_SCALE),
                          Qt::AlignLeft, tr(caption_.toStdString().c_str()));

        painter->setOpacity(OPACITY_HALF);
        QString t;
        if (text_.isEmpty())
        {
            t = "--";
        }
        else if (!maskingChar_.isNull())
        {
            t = QString(text_.size(), maskingChar_);
        }
        else
        {
            t = text_;
        }

        painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, 
                                                  PREFERENCES_MARGIN*G_SCALE,
                                                  -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                                  -PREFERENCES_MARGIN),
                          Qt::AlignRight, t);
    }
}

void EditBoxItem::setText(const QString &text)
{
    text_ = text;
    lineEdit_->setText(text_);
    update();
}

void EditBoxItem::setValidator(QRegularExpressionValidator *validator)
{
    lineEdit_->setValidator(validator);
}

void EditBoxItem::setEditButtonClickable(bool clickable)
{
    btnEdit_->setClickable(clickable);
}

void EditBoxItem::setMasked(bool masked)
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

bool EditBoxItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void EditBoxItem::keyPressEvent(QKeyEvent *event)
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

void EditBoxItem::onEditClick()
{
    btnEdit_->hide();
    btnConfirm_->show();
    btnUndo_->show();
    proxyWidget_->show();
    lineEdit_->setFocus();
    isEditMode_ = true;
    update();
}

void EditBoxItem::onConfirmClick()
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

void EditBoxItem::onUndoClick()
{
    btnEdit_->show();
    btnConfirm_->hide();
    btnUndo_->hide();
    proxyWidget_->hide();
    isEditMode_ = false;
    update();
}

void EditBoxItem::updateScaling()
{
    CommonGraphics::BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

void EditBoxItem::onLanguageChanged()
{
    lineEdit_->setPlaceholderText(tr(editPlaceholderText_.toStdString().c_str()));
}

void EditBoxItem::updatePositions()
{
    btnEdit_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN)*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
    btnConfirm_->setPos(boundingRect().width() - (ICON_WIDTH + PREFERENCES_MARGIN)*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
    btnUndo_->setPos(boundingRect().width() - (2*ICON_WIDTH + 2*PREFERENCES_MARGIN)*G_SCALE, PREFERENCES_MARGIN*G_SCALE);
    lineEdit_->setFont(*FontManager::instance().getFont(12, true));

    if (!proxyWidget_->isVisible()) // workaround Qt bug (setGeometry not working when proxyWidget_ is not visible)
    {
        proxyWidget_->show();
        lineEdit_->setGeometry(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, boundingRect().width() - (2*ICON_WIDTH + 3*PREFERENCES_MARGIN)*G_SCALE, ICON_HEIGHT*G_SCALE);
        proxyWidget_->hide();
    }
    else
    {
        lineEdit_->setGeometry(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, boundingRect().width() - (2*ICON_WIDTH + 3*PREFERENCES_MARGIN)*G_SCALE, ICON_HEIGHT*G_SCALE);
    }
}

} // namespace PreferencesWindow

