#include "upgradewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace UpgradeWindow {

UpgradeWindowItem::UpgradeWindowItem(Preferences *preferences, ScalableGraphicsObject *parent) :
    ScalableGraphicsObject(parent), preferences_(preferences), height_(WINDOW_HEIGHT)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences_, &Preferences::appSkinChanged, this, &UpgradeWindowItem::onAppSkinChanged);

    // accept
    acceptButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kBright, 128, 40, 20);
    connect(acceptButton_, &CommonGraphics::BubbleButton::clicked, this, &UpgradeWindowItem::acceptClick);
    // cancel
    double cancelOpacity = OPACITY_UNHOVER_TEXT;
    cancelButton_ = new CommonGraphics::TextButton("", FontDescr(16, QFont::Normal), Qt::white, true, this);
    connect(cancelButton_, &CommonGraphics::TextButton::clicked, this, &UpgradeWindowItem::cancelClick);
    cancelButton_->quickSetOpacity(cancelOpacity);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &UpgradeWindowItem::onLanguageChanged);
    onLanguageChanged();

    updatePositions();
}

QRectF UpgradeWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, height_*G_SCALE);
}

void UpgradeWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        painter->setPen(Qt::NoPen);
        QPainterPath path;
        path.addRoundedRect(boundingRect().toRect(), 9*G_SCALE, 9*G_SCALE);
        painter->fillPath(path, QColor(9, 15, 25));
        painter->setPen(Qt::SolidLine);
    } else {
        QString background = "background/MAIN_BG";
        painter->setOpacity(OPACITY_FULL * initialOpacity);
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(background);
        p->draw(0, 0, painter);

        QString borderInner = "background/MAIN_BORDER_INNER";
        QSharedPointer<IndependentPixmap> borderInnerPixmap = ImageResourcesSvg::instance().getIndependentPixmap(borderInner);
        borderInnerPixmap->draw(0, 0, WINDOW_WIDTH*G_SCALE, (WINDOW_HEIGHT-1)*G_SCALE, painter);
    }

    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -16*G_SCALE : 0;

    // title
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    painter->setPen(FontManager::instance().getErrorRedColor());
    QFont titleFont = FontManager::instance().getFont(24, QFont::Bold);
    painter->setFont(titleFont);
    QRectF titleRect(0, (TITLE_POS_Y + yOffset)*G_SCALE, WINDOW_WIDTH*G_SCALE, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr("You're out of data!"));

    // main description
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    QFont descFont(FontManager::instance().getFont(14,  QFont::Normal));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    const QString descText = tr("Don't leave your front door open. "
                                "Upgrade or wait until next month to get your monthly data allowance back.");
    int width = CommonGraphics::idealWidthOfTextRect(DESCRIPTION_WIDTH_MIN*G_SCALE, WINDOW_WIDTH*G_SCALE, 3, descText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, width), (DESCRIPTION_POS_Y + yOffset)*G_SCALE,
                      width, height_*G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descText);

}

void UpgradeWindowItem::updateScaling()
{
    updatePositions();
}

void UpgradeWindowItem::updatePositions()
{
    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -14*G_SCALE : 0;

    acceptButton_->updateScaling();
    int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, (ACCEPT_BUTTON_POS_Y + yOffset)*G_SCALE);

    cancelButton_->updateScaling();
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, cancelButton_->boundingRect().width());
    cancelButton_->setPos(cancelPosX, (CANCEL_BUTTON_POS_Y + yOffset)*G_SCALE);
}

void UpgradeWindowItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit cancelClick();
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        emit acceptClick();
    }
}

void UpgradeWindowItem::onLanguageChanged()
{
    acceptButton_->setText(tr("Get more data"));
    cancelButton_->setText(tr("I'm broke"));
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH*G_SCALE, cancelButton_->boundingRect().width());
    int yOffset = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? -14*G_SCALE : 0;
    cancelButton_->setPos(cancelPosX, (CANCEL_BUTTON_POS_Y + yOffset)*G_SCALE);
}

void UpgradeWindowItem::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    update();
}

void UpgradeWindowItem::setHeight(int height)
{
    prepareGeometryChange();
    height_ = height;
    updatePositions();
}

} // namespace
