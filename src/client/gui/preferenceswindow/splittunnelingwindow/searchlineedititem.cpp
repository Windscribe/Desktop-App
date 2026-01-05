#include "searchlineedititem.h"

#include <QPainter>
#include <QLineEdit>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "preferenceswindow/preferencesconst.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SearchLineEditItem::SearchLineEditItem(ScalableGraphicsObject *parent)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), searchIconOpacity_(OPACITY_HALF), editing_(false)
{
    closeButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "CLOSE_ICON", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(closeButton_, &IconButton::clicked, this, &SearchLineEditItem::onCancelClicked);

    clearTextButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/CLEAR_ICON", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(clearTextButton_, &IconButton::clicked, this, &SearchLineEditItem::onClearTextClicked);

    QString placeHolderText = tr("Search");
    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(placeHolderText);
    lineEdit_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);
    connect(lineEdit_, &CommonWidgets::CustomMenuLineEdit::focusIn, this, &SearchLineEditItem::focusIn);
    connect(lineEdit_, &CommonWidgets::CustomMenuLineEdit::textChanged, this, &SearchLineEditItem::onLineEditTextChanged);
    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SearchLineEditItem::onLanguageChanged);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);

    connect(&searchIconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &SearchLineEditItem::onSearchIconOpacityChanged);
    updatePositions();
}

void SearchLineEditItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(searchIconOpacity_);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("preferences/MAGNIFYING_GLASS");
    p->draw(PREFERENCES_MARGIN_X*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - APP_ICON_HEIGHT)*G_SCALE / 2, APP_ICON_WIDTH*G_SCALE, APP_ICON_HEIGHT*G_SCALE, painter);

    // vertical divider
    painter->setOpacity(OPACITY_DIVIDER_LINE);
    painter->fillRect(boundingRect().width() - (1.5*PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE, VERTICAL_DIVIDER_MARGIN_Y*G_SCALE, 1*G_SCALE, VERTICAL_DIVIDER_HEIGHT*G_SCALE, Qt::white);
}

void SearchLineEditItem::hideButtons()
{
    clearTextButton_->hide();
}

void SearchLineEditItem::showButtons()
{
    clearTextButton_->show();
}

QString SearchLineEditItem::text()
{
    return lineEdit_->text();
}

void SearchLineEditItem::setText(QString text)
{
    return lineEdit_->setText(text);
}

void SearchLineEditItem::setFocusOnSearchBar()
{
    lineEdit_->setFocus();
}

void SearchLineEditItem::setSelected(bool selected)
{
    selected_ = selected;

    double targetOpacity = OPACITY_FULL;
    if (!selected) targetOpacity = OPACITY_HALF;
    startAnAnimation<double>(searchIconOpacityAnimation_, searchIconOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

void SearchLineEditItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
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
    closeButton_->setPos(boundingRect().width() - (PREFERENCES_MARGIN_X + ICON_WIDTH)*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2);
    clearTextButton_->setPos(boundingRect().width() - (2*PREFERENCES_MARGIN_X + 2*ICON_WIDTH + 1)*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2);
    lineEdit_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    lineEdit_->setGeometry((PREFERENCES_MARGIN_X + APP_ICON_MARGIN_X + APP_ICON_WIDTH)*G_SCALE, (PREFERENCE_GROUP_ITEM_HEIGHT - ICON_HEIGHT)*G_SCALE / 2, 128*G_SCALE, ICON_HEIGHT*G_SCALE);
}

}
