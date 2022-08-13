#include "searchlineedititem.h"

#include <QPainter>
#include <QLineEdit>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "languagecontroller.h"
#include "../dividerline.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SearchLineEditItem::SearchLineEditItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
    , searchIconOpacity_(OPACITY_UNHOVER_ICON_STANDALONE)
    , editing_(false)
{
    closeButton_ = new IconButton(16, 16, "preferences/CLOSE_ICON", "", this, OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    closeButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    closeButton_->setHoverOpacity(OPACITY_FULL);
    connect(closeButton_, SIGNAL(clicked()), SLOT(onCancelClicked()));

    clearTextButton_ = new IconButton(16, 16, "preferences/CLEAR_ICON", "", this,OPACITY_UNHOVER_ICON_STANDALONE,OPACITY_FULL);
    clearTextButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    clearTextButton_->setHoverOpacity(OPACITY_FULL);
    connect(clearTextButton_, SIGNAL(clicked()), SLOT(onClearTextClicked()));

    QString placeHolderText = tr("Search");
    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(placeHolderText);
    lineEdit_->setFont(*FontManager::instance().getFont(14, false));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);
    connect(lineEdit_, SIGNAL(focusIn()), SIGNAL(focusIn()));

    connect(lineEdit_, SIGNAL(textChanged(QString)), SLOT(onLineEditTextChanged(QString)));
    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);

    line_ = new DividerLine(this, 276);

    connect(&searchIconOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSearchIconOpacityChanged(QVariant)));
    updatePositions();
}

void SearchLineEditItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setOpacity(searchIconOpacity_ * initOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/VIEW_LOG_ICON");
    p->draw(16*G_SCALE, 14*G_SCALE, painter);
}

void SearchLineEditItem::hideButtons()
{
    clearTextButton_->hide();
}

void SearchLineEditItem::showButtons()
{
    clearTextButton_->show();
}

QString SearchLineEditItem::getText()
{
    return lineEdit_->text();
}

void SearchLineEditItem::setFocusOnSearchBar()
{
    lineEdit_->setFocus();
}

void SearchLineEditItem::setSelected(bool selected)
{
    selected_ = selected;

    double targetOpacity = OPACITY_FULL;
    if (!selected) targetOpacity = OPACITY_UNHOVER_ICON_STANDALONE;
    startAnAnimation<double>(searchIconOpacityAnimation_, searchIconOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void SearchLineEditItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();
}

bool SearchLineEditItem::hasItemWithFocus()
{
    return lineEdit_->hasFocus();
}

void SearchLineEditItem::focusInEvent(QFocusEvent * /*event*/)
{
    emit focusIn();
}

void SearchLineEditItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        if (lineEdit_->text() == "")
        {
            exitSearchMode();
        }
        else
        {
            lineEdit_->setText("");
        }
    }
    else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        emit enterPressed();
    }
}

void SearchLineEditItem::onCancelClicked()
{
    exitSearchMode();
}

void SearchLineEditItem::onClearTextClicked()
{
    lineEdit_->setText("");
    proxyWidget_->setFocus();
}

void SearchLineEditItem::onLanguageChanged()
{
    QString placeHolderText = tr("Search");
    lineEdit_->setPlaceholderText(placeHolderText);
}

void SearchLineEditItem::onLineEditTextChanged(QString text)
{
    emit textChanged(text);
}

void SearchLineEditItem::onSearchIconOpacityChanged(const QVariant &value)
{
    searchIconOpacity_ = value.toDouble();
    update();
}

void SearchLineEditItem::exitSearchMode()
{
    lineEdit_->setText("");
    emit searchModeExited();
}

void SearchLineEditItem::updatePositions()
{
    closeButton_->setPos(boundingRect().width() - 32*G_SCALE, boundingRect().height()/2 - 12*G_SCALE);
    clearTextButton_->setPos(boundingRect().width() - 58*G_SCALE, boundingRect().height()/2 - 12*G_SCALE);
    lineEdit_->setFont(*FontManager::instance().getFont(14, false));
    lineEdit_->setGeometry(40*G_SCALE, 2*G_SCALE, 178*G_SCALE, 40*G_SCALE);
    line_->setPos(24*G_SCALE, 43*G_SCALE);
}

}
