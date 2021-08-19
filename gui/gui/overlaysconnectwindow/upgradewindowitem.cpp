#include "upgradewindowitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace UpgradeWindow {

UpgradeWindowItem::UpgradeWindowItem(ScalableGraphicsObject *parent) :
    ScalableGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    // accept
    acceptButton_ = new CommonGraphics::BubbleButtonBright(this, 128, 40, 20, 20);
    connect(acceptButton_, SIGNAL(clicked()), this, SIGNAL(acceptClick()));
    QString upgradeText = QT_TRANSLATE_NOOP("CommonGraphics::BubbleButtonBright", "Get more data");
    acceptButton_->setText(upgradeText);

    // cancel
    QString cancelText = QT_TRANSLATE_NOOP("CommonGraphics::TextButton", "I'm broke");
    double cancelOpacity = OPACITY_UNHOVER_TEXT;
    cancelButton_ = new CommonGraphics::TextButton(cancelText, FontDescr(16, false),
                                                   Qt::white, true, this);
    connect(cancelButton_, SIGNAL(clicked()), this, SIGNAL(cancelClick()));
    cancelButton_->quickSetOpacity(cancelOpacity);

    updatePositions();
}

QRectF UpgradeWindowItem::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH * G_SCALE, WINDOW_HEIGHT * G_SCALE);
}

void UpgradeWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // background:

#ifdef Q_OS_WIN
    QString background = "background/WIN_MAIN_BG";
#else
    QString background = "background/MAC_MAIN_BG";
#endif
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(background);
    p->draw(0, 0, painter);

    // title
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    painter->setPen(FontManager::instance().getErrorRedColor());
    QFont titleFont = *FontManager::instance().getFont(24, true);
    painter->setFont(titleFont);
    QRectF titleRect(0, TITLE_POS_Y * G_SCALE, WINDOW_WIDTH * G_SCALE, CommonGraphics::textHeight(titleFont));
    painter->drawText(titleRect, Qt::AlignCenter, tr("You're out of data!"));

    // main description
    painter->setOpacity(OPACITY_FULL * initialOpacity);
    QFont descFont(*FontManager::instance().getFont(14, false));
    painter->setFont(descFont);
    painter->setPen(Qt::white);

    const QString descText = tr("Don't leave your front door open. "
                             "Upgrade or wait until next month to get your monthly data allowance back.");
    int width = CommonGraphics::idealWidthOfTextRect(DESCRIPTION_WIDTH_MIN * G_SCALE, WINDOW_WIDTH * G_SCALE, 3, descText, descFont);
    painter->drawText(CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, width), DESCRIPTION_POS_Y * G_SCALE,
                      width, WINDOW_HEIGHT * G_SCALE,
                      Qt::AlignHCenter | Qt::TextWordWrap, descText);

}

void UpgradeWindowItem::updateScaling()
{
    updatePositions();
}

void UpgradeWindowItem::updatePositions()
{
    acceptButton_->updateScaling();
    int acceptPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, acceptButton_->boundingRect().width());
    acceptButton_->setPos(acceptPosX, ACCEPT_BUTTON_POS_Y * G_SCALE);

    cancelButton_->updateScaling();
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y * G_SCALE);
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
    cancelButton_->recalcBoundingRect();
    int cancelPosX = CommonGraphics::centeredOffset(WINDOW_WIDTH * G_SCALE, cancelButton_->getWidth());
    cancelButton_->setPos(cancelPosX, CANCEL_BUTTON_POS_Y * G_SCALE);
}

} // namespace
