#include "newaddressitem.h"

#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

NewAddressItem::NewAddressItem(ScalableGraphicsObject *parent)
  : BaseItem (parent, PREFERENCE_GROUP_ITEM_HEIGHT), editing_(false)
{
    addAddressButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "locations/EXPAND_ICON", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(addAddressButton_, &IconButton::clicked, this, &NewAddressItem::onAddAddressClicked);

    cancelTextButton_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/CLEAR_ICON", "", this, OPACITY_HALF, OPACITY_FULL);
    connect(cancelTextButton_, &IconButton::clicked, this, &NewAddressItem::onCancelClicked);

    QString placeHolderText = tr("Enter IP or Hostname");
    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(placeHolderText);
    lineEdit_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);
    connect(lineEdit_, &CommonWidgets::CustomMenuLineEdit::textChanged, this, &NewAddressItem::onLineEditTextChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &NewAddressItem::onLanguageChanged);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);

    setEditMode(false);
    updatePositions();
}

void NewAddressItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void NewAddressItem::setSelected(bool selected)
{
    selected_ = selected;

    if (selected)
    {
        lineEdit_->setFocus();
    }
}

void NewAddressItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
    updatePositions();
}

bool NewAddressItem::lineEditHasFocus()
{
    return lineEdit_->hasFocus();
}

void NewAddressItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        submitEntryForRule();
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (lineEdit_->text() == "")
        {
            emit escape();
        }
        else
        {
            lineEdit_->setText("");
        }
    }
}

void NewAddressItem::setEditMode(bool editMode)
{
    editing_ = editMode;

    cancelTextButton_->setClickable(editMode);

    if (editMode)
    {
        cancelTextButton_->show();
    }
    else
    {
        cancelTextButton_->hide();
    }
}

void NewAddressItem::onCancelClicked()
{
    lineEdit_->setText("");
}

void NewAddressItem::onLanguageChanged()
{
    QString placeHolderText = tr("Enter IP or Hostname");
    lineEdit_->setPlaceholderText(placeHolderText);
}

void NewAddressItem::onLineEditTextChanged(QString text)
{
    setEditMode(text != "");
    emit keyPressed();
}

void NewAddressItem::submitEntryForRule()
{
    QString input = lineEdit_->text();
    setEditMode(false);
    lineEdit_->setText("");
    emit addNewAddressClicked(input);
}

void NewAddressItem::updatePositions()
{
    addAddressButton_->setPos(boundingRect().width() - ICON_WIDTH*G_SCALE - PREFERENCES_MARGIN_X*G_SCALE, boundingRect().height()/2 - ICON_HEIGHT/2*G_SCALE);
    cancelTextButton_->setPos(boundingRect().width() - 2*ICON_WIDTH*G_SCALE - 2*PREFERENCES_MARGIN_X*G_SCALE, boundingRect().height()/2 - ICON_HEIGHT/2*G_SCALE);
    lineEdit_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    lineEdit_->setGeometry(PREFERENCES_MARGIN_X*G_SCALE, 0, boundingRect().width() - 2*ICON_WIDTH*G_SCALE - 4*PREFERENCES_MARGIN_X*G_SCALE, boundingRect().height());
}

void NewAddressItem::onAddAddressClicked()
{
    submitEntryForRule();
}

void NewAddressItem::setClickable(bool clickable)
{
    addAddressButton_->setClickable(clickable);
    lineEdit_->setClickable(clickable);
}

}
