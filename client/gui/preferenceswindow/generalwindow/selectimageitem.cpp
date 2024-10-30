#include "selectimageitem.h"

#include <QFileDialog>
#include <QPainter>
#include "commongraphics/basepage.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "showingdialogstate.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

SelectImageItem::SelectImageItem(ScalableGraphicsObject *parent, const QString &caption)
  : CommonGraphics::BaseItem(parent, SELECT_IMAGE_HEIGHT*G_SCALE), caption_(caption)
{
    btnOpen_ = new IconButton(ICON_WIDTH, ICON_HEIGHT, "preferences/EDIT_ICON", "", this);
    connect(btnOpen_, &IconButton::clicked, this, &SelectImageItem::onOpenClick);

    path_ = "filename1.png";

    updatePositions();
}

void SelectImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // caption
    QFont font = FontManager::instance().getFont(12, false);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL);

    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                              SELECT_IMAGE_MARGIN_Y*G_SCALE,
                                              -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                              -SELECT_IMAGE_MARGIN_Y*G_SCALE),
                      Qt::AlignLeft,
                      caption_);

    // (664x274) (at 50% opacity)
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(caption_);
    int textHeight = fm.height();

    painter->setOpacity(OPACITY_HALF);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE + textWidth,
                                              SELECT_IMAGE_MARGIN_Y*G_SCALE,
                                              -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                              -SELECT_IMAGE_MARGIN_Y*G_SCALE),
                      Qt::AlignLeft,
                      " (664x274)");

    // path
    QRectF rcText = boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE,
                                            SELECT_IMAGE_MARGIN_Y*G_SCALE + textHeight,
                                            -(2*PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                                            -SELECT_IMAGE_MARGIN_Y*G_SCALE);
    if (filenameForShow_.isEmpty())
    {
        painter->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter, tr("[no selection]"));
    }
    else
    {
        QString elidedPath = fm.elidedText(filenameForShow_, Qt::ElideRight, rcText.width());
        painter->drawText(rcText, Qt::AlignLeft | Qt::AlignVCenter, elidedPath);
    }
}

void SelectImageItem::setPath(const QString &path)
{
    path_ = path;
    QFileInfo fileInfo(path);
    filenameForShow_ = fileInfo.fileName();
    update();
}

void SelectImageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setHeight(SELECT_IMAGE_HEIGHT*G_SCALE);
    updatePositions();
}

void SelectImageItem::onOpenClick()
{
    QString filename;
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
    filename = QFileDialog::getOpenFileName(g_mainWindow, tr("Select an image"), QString(), "Images (*.png *.jpg *.gif)");
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

void SelectImageItem::setCaption(const QString &caption)
{
    caption_ = caption;
    update();
}

void SelectImageItem::updatePositions()
{
    btnOpen_->setPos(boundingRect().width() - (PREFERENCES_MARGIN + ICON_WIDTH)*G_SCALE,
                     (SELECT_IMAGE_HEIGHT - ICON_HEIGHT)/2*G_SCALE);
}

} // namespace PreferencesWindow

