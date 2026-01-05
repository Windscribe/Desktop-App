#include "credentiallineedit.h"

#include <QLineEdit>
#include <QPainter>
#include <QTimer>
#include "commongraphics/commongraphics.h"
#include "commonwidgets/custommenulineedit.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

namespace GeneralMessageWindow {

CredentialLineEdit::CredentialLineEdit(ScalableGraphicsObject *parent, const QString &text, bool isPassword)
    : BaseItem(parent, 58*G_SCALE, "", true, WINDOW_WIDTH - 72), text_(text)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    setClickable(false);

    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setGeometry(12*G_SCALE, 24*G_SCALE, (WINDOW_WIDTH - 96)*G_SCALE, 34*G_SCALE);
    lineEdit_->setColorScheme(true);
    connect(lineEdit_, &CommonWidgets::CustomMenuLineEdit::editingFinished, this, &CredentialLineEdit::editingFinished);
    updateStyleSheet();
    if (isPassword) {
        lineEdit_->setEchoMode(QLineEdit::Password);
        lineEdit_->setShowRevealToggle(true);
    }

    proxy_ = new QGraphicsProxyWidget(this);
    proxy_->setWidget(lineEdit_);
}

void CredentialLineEdit::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font(FontManager::instance().getFont(14, QFont::Normal));
    painter->setPen(Qt::white);
    painter->drawText(QRect(12*G_SCALE, 0, (WINDOW_WIDTH-96)*G_SCALE, 16*G_SCALE), Qt::AlignTop | Qt::AlignLeft, text_);
}

void CredentialLineEdit::updateScaling()
{
    setHeight(58*G_SCALE);
    updateStyleSheet();
    update();
}

QString CredentialLineEdit::text() const
{
    return lineEdit_->text();
}

void CredentialLineEdit::setLineEditText(const QString &text)
{
    lineEdit_->setText(text);
}

void CredentialLineEdit::updateStyleSheet()
{
    lineEdit_->setStyleSheet("border-radius: 3px; background: #26ffffff; color: white; padding: 0 8px 0 8px;");
}

void CredentialLineEdit::clear()
{
    lineEdit_->clear();
}

void CredentialLineEdit::focusInEvent(QFocusEvent *event)
{
    QTimer::singleShot(0, [this]() {
        lineEdit_->setFocus();
    });
}

void CredentialLineEdit::focusOutEvent(QFocusEvent *event)
{
    lineEdit_->clearFocus();
}

} // namespace GeneralMessageWindow
