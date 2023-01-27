#include "editboxitem.h"

#include <QPainter>
#include <QStyleOption>
#include "basepage.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

EditBoxItem::EditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt, bool isDrawFullBottomDivider) :
    ScalableGraphicsObject(parent),
    caption_(caption),
    isEditMode_(false),
    maskingChar_(0)
{
    line_ = new DividerLine(this, isDrawFullBottomDivider ? 276 : 264);

    btnEdit_ = new IconButton(16, 16, "preferences/EDIT_ICON", "", this);
    connect(btnEdit_, SIGNAL(clicked()), SLOT(onEditClick()));

    btnConfirm_ = new IconButton(16, 16, "preferences/CONFIRM_ICON", "", this);
    btnConfirm_->hide();
    connect(btnConfirm_, SIGNAL(clicked()), SLOT(onConfirmClick()));

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

QRectF EditBoxItem::boundingRect() const
{
    return QRectF(0, 0, PAGE_WIDTH*G_SCALE, 43*G_SCALE);
}

void EditBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    qreal initialOpacity = painter->opacity();

    painter->fillRect(boundingRect().adjusted(24*G_SCALE, 0, 0, 0), QBrush(QColor(16, 22, 40)));

    if (!isEditMode_)
    {
        QFont *font = FontManager::instance().getFont(12, true);
        painter->setFont(*font);
        painter->setPen(QColor(255, 255, 255));
        painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, 0, -3*G_SCALE), Qt::AlignVCenter, tr(caption_.toStdString().c_str()));

        painter->setOpacity(0.5 * initialOpacity);
        QString t;
        if (text_.isEmpty())
            t = "--";
        else if (maskingChar_ != 0)
            t = QString(text_.size(), maskingChar_);
        else
            t = text_;

        int rightOffs = -48*G_SCALE;
        painter->drawText(boundingRect().adjusted(40*G_SCALE, 0, rightOffs, -3*G_SCALE), Qt::AlignRight | Qt::AlignVCenter, t);
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

void EditBoxItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
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
        maskingChar_ = QChar(
            lineEdit_->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, lineEdit_)
        );
        lineEdit_->setEchoMode(QLineEdit::Password);
    } else {
        maskingChar_ = QChar(0);
        lineEdit_->setEchoMode(QLineEdit::Normal);
    }
    update();
}

bool EditBoxItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void EditBoxItem::keyReleaseEvent(QKeyEvent *event)
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


void EditBoxItem::onLanguageChanged()
{
    lineEdit_->setPlaceholderText(tr(editPlaceholderText_.toStdString().c_str()));
}

void EditBoxItem::updatePositions()
{
    line_->setPos(24*G_SCALE, 40*G_SCALE);
    btnEdit_->setPos(boundingRect().width() - btnEdit_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnEdit_->boundingRect().height()) / 2);

    btnConfirm_->setPos(boundingRect().width() - btnConfirm_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnConfirm_->boundingRect().height()) / 2);
    btnUndo_->setPos(boundingRect().width() - btnUndo_->boundingRect().width() - (16 + 32)*G_SCALE, (boundingRect().height() - line_->boundingRect().height() - btnUndo_->boundingRect().height()) / 2);
    lineEdit_->setFont(*FontManager::instance().getFont(12, true));

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

