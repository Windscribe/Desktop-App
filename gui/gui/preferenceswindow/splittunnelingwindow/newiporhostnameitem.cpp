#include "newiporhostnameitem.h"

#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "../dividerline.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

NewIpOrHostnameItem::NewIpOrHostnameItem(ScalableGraphicsObject *parent) : BaseItem (parent, 50)
    , editing_(false)
{
    addIpOrHostnameButton_ = new IconButton(16, 16, "locations/EXPAND_ICON", this, OPACITY_UNHOVER_ICON_STANDALONE, OPACITY_FULL);
    addIpOrHostnameButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    addIpOrHostnameButton_->setHoverOpacity(OPACITY_FULL);
    connect(addIpOrHostnameButton_, SIGNAL(clicked()), SLOT(onAddIpOrHostnameClicked()));

    cancelTextButton_ = new IconButton(16, 16, "preferences/CLEAR_ICON", this, OPACITY_UNHOVER_ICON_STANDALONE, OPACITY_FULL);
    cancelTextButton_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
    cancelTextButton_->setHoverOpacity(OPACITY_FULL);
    connect(cancelTextButton_, SIGNAL(clicked()), SLOT(onCancelClicked()));

    QString placeHolderText = tr("Enter IP or Hostname");
    lineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    lineEdit_->setPlaceholderText(placeHolderText);
    lineEdit_->setFont(*FontManager::instance().getFont(14, false));
    lineEdit_->setStyleSheet("background: transparent; color: rgb(135, 138, 147)");
    lineEdit_->setFrame(false);
    connect(lineEdit_, SIGNAL(textChanged(QString)), SLOT(onLineEditTextChanged(QString)));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(lineEdit_);

    line_ = new DividerLine(this, 276);

    setEditMode(false);
    updatePositions();
}

void NewIpOrHostnameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void NewIpOrHostnameItem::setSelected(bool selected)
{
    selected_ = selected;

    if (selected)
    {
        lineEdit_->setFocus();
    }
}

void NewIpOrHostnameItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    updatePositions();

}

void NewIpOrHostnameItem::keyReleaseEvent(QKeyEvent *event)
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

void NewIpOrHostnameItem::setEditMode(bool editMode)
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

void NewIpOrHostnameItem::onCancelClicked()
{
    lineEdit_->setText("");
}

void NewIpOrHostnameItem::onLanguageChanged()
{
    QString placeHolderText = tr("Enter IP or Hostname");
    lineEdit_->setPlaceholderText(placeHolderText);
}

void NewIpOrHostnameItem::onLineEditTextChanged(QString text)
{
    setEditMode(text != "");
}

void NewIpOrHostnameItem::submitEntryForRule()
{
    QString input = lineEdit_->text();
    setEditMode(false);
    lineEdit_->setText("");
    emit addNewIpOrHostnameClicked(input);
}

void NewIpOrHostnameItem::updatePositions()
{
    addIpOrHostnameButton_->setPos(boundingRect().width() - 32*G_SCALE, boundingRect().height()/2 - 12*G_SCALE);
    cancelTextButton_->setPos(boundingRect().width() - 58*G_SCALE, boundingRect().height()/2 - 12*G_SCALE);
    lineEdit_->setFont(*FontManager::instance().getFont(14, false));
    lineEdit_->setGeometry(16*G_SCALE, 0, 194 * G_SCALE, 40 * G_SCALE);
    line_->setPos(24*G_SCALE, 43*G_SCALE);
}

void NewIpOrHostnameItem::onAddIpOrHostnameClicked()
{
    submitEntryForRule();
}

}
