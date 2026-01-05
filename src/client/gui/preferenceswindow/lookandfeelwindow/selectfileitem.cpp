#include "selectfileitem.h"

#include <QFileDialog>
#include <QPainter>
#include "commongraphics/basepage.h"
#include "commongraphics/texticonbutton.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "showingdialogstate.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

SelectFileItem::SelectFileItem(ScalableGraphicsObject *parent) : CommonGraphics::BaseItem(parent, kSelectImageHeight*G_SCALE)
{
    button_ = new CommonGraphics::TextIconButton(4, "", "preferences/EDIT_SMALL_ICON", this, true);
    button_->setFont(FontDescr(12, QFont::Normal));
    button_->setPos((PREFERENCES_MARGIN_X - 4)*G_SCALE, 0);
    connect(button_, &CommonGraphics::TextIconButton::clicked, this, &SelectFileItem::onClick);

    updatePositions();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SelectFileItem::onLanguageChanged);
    onLanguageChanged();
}

QRectF SelectFileItem::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void SelectFileItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void SelectFileItem::setPath(const QString &path)
{
    path_ = path;
    QFileInfo fileInfo(path);
    filename_ = fileInfo.fileName();

    if (filename_.isEmpty()) {
        button_->setText(tr("[no selection]"));
    } else {
        button_->setText(filename_);
    }

    updatePositions();
}

void SelectFileItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setHeight(kSelectImageHeight*G_SCALE);
    updatePositions();
}

void SelectFileItem::onClick()
{
    QString filename;
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
    filename = QFileDialog::getOpenFileName(g_mainWindow, dialogTitle_, QString(), dialogFilter_);
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    if (!filename.isEmpty())
    {
        QString prevPath = path_;
        setPath(filename);
        if (prevPath != path_)
        {
            emit pathChanged(path_);
        }
    }
}

void SelectFileItem::updatePositions()
{
    button_->setPos((PREFERENCES_MARGIN_X - 4)*G_SCALE, (kSelectImageHeight - button_->getHeight()) / 2);
    button_->setMaxWidth(maxWidth_);

    if (width_ != maxWidth_) {
        width_ = maxWidth_;
        prepareGeometryChange();
    }
    update();
}

void SelectFileItem::setDialogText(const QString &title, const QString &filter)
{
    dialogTitle_ = title;
    dialogFilter_ = filter;
}

void SelectFileItem::onLanguageChanged()
{
    if (filename_.isEmpty()) {
        button_->setText(tr("[no selection]"));
    }
}

void SelectFileItem::setMaxWidth(int maxWidth)
{
    maxWidth_ = maxWidth;
    updatePositions();
}

} // namespace PreferencesWindow
