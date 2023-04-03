#include "generalmessageitem.h"

#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

namespace GeneralMessageWindow {

GeneralMessageItem::GeneralMessageItem(ScalableGraphicsObject *parent, int width, IGeneralMessageWindow::Style style)
    : CommonGraphics::BasePage(parent, width), style_(style), shape_(IGeneralMessageWindow::kLoginScreenShape),
    title_(""), desc_(""), titleSize_(16), acceptButton_(nullptr), rejectButton_(nullptr), selection_(NONE)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(kSpacerHeight);
    setIndent(kIndent);

    updateScaling();
}

void GeneralMessageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // title
    painter->setOpacity(OPACITY_FULL);
    QFont *titleFont = FontManager::instance().getFont(titleSize_, true);
    painter->setFont(*titleFont);
    painter->setPen(QColor(Qt::white));

    painter->drawText(boundingRect().adjusted(36*G_SCALE,
                                              ((icon_ != nullptr) ? icon_->boundingRect().height() : 0) + 16*G_SCALE,
                                              -36*G_SCALE,
                                              0),
                      Qt::AlignHCenter | Qt::TextWordWrap, title_);

    // description
    if (!desc_.isEmpty()) {
        painter->setOpacity(OPACITY_HALF);
        QFont *descFont = FontManager::instance().getFont(14, false);
        painter->setFont(*descFont);

        // 60 for icon + 24 spacing + title + 16 spacing
        painter->drawText(boundingRect().adjusted(36*G_SCALE,
                                                  titleHeight_ + ((icon_ != nullptr) ? icon_->boundingRect().height() : 0) + 40*G_SCALE,
                                                  -36*G_SCALE,
                                                  0),
                          Qt::AlignHCenter | Qt::TextWordWrap, desc_);

    }
}

void GeneralMessageItem::setIcon(const QString &icon)
{
    if (!icon.isEmpty()) {
        icon_.reset(new ImageItem(this, icon));
        updatePositions();
    } else {
        icon_.reset();
    }

    update();
}

void GeneralMessageItem::setTitle(const QString &title)
{
    title_ = title;
    QFont *font = FontManager::instance().getFont(titleSize_, true);
    QFontMetrics fm(*font);
    titleHeight_ = fm.boundingRect(boundingRect().adjusted(36*G_SCALE, 0, -36*G_SCALE, 0).toRect(),
                                   Qt::AlignHCenter | Qt::TextWordWrap, title_).height();

    updatePositions();
}

void GeneralMessageItem::setDescription(const QString &desc)
{
    desc_ = desc;
    QFont *font = FontManager::instance().getFont(14, true);
    QFontMetrics fm(*font);
    descHeight_ = fm.boundingRect(boundingRect().adjusted(36*G_SCALE, 0, -36*G_SCALE, 0).toRect(),
                                  Qt::AlignHCenter | Qt::TextWordWrap, desc_).height();

    updatePositions();
}

void GeneralMessageItem::setAcceptText(const QString &text)
{
    if (!acceptButton_) {
        CommonGraphics::ListButton::Style buttonStyle = CommonGraphics::ListButton::kBright;
        if (style_ == IGeneralMessageWindow::kDark) {
            buttonStyle = CommonGraphics::ListButton::kDark;
        }
        acceptButton_ = new CommonGraphics::ListButton(this, buttonStyle, text);
        connect(acceptButton_, &CommonGraphics::ListButton::clicked, this, &GeneralMessageItem::acceptClick);
        connect(acceptButton_, &CommonGraphics::ListButton::hoverEnter, this, &GeneralMessageItem::onHoverAccept);
        connect(acceptButton_, &CommonGraphics::ListButton::hoverLeave, this, &GeneralMessageItem::onHoverLeaveAccept);
        addItem(acceptButton_);
    } else {
        acceptButton_->setText(text);
    }
}

void GeneralMessageItem::setRejectText(const QString &text)
{
    if (!rejectButton_) {
        rejectButton_ = new CommonGraphics::ListButton(this, CommonGraphics::ListButton::kText, text);
        connect(rejectButton_, &CommonGraphics::ListButton::clicked, this, &GeneralMessageItem::rejectClick);
        connect(rejectButton_, &CommonGraphics::ListButton::hoverEnter, this, &GeneralMessageItem::onHoverReject);
        connect(rejectButton_, &CommonGraphics::ListButton::hoverLeave, this, &GeneralMessageItem::onHoverLeaveReject);
        addItem(rejectButton_);
    } else {
        rejectButton_->setText(text);
    }
}

void GeneralMessageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void GeneralMessageItem::keyPressEvent(QKeyEvent *event)
{
    if (!isVisible()) {
        // don't handle key events if invisible
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (selection_ == ACCEPT) {
            emit acceptClick();
        } else if (selection_ == REJECT) {
            emit rejectClick();
        }
    } else if (event->key() == Qt::Key_Escape) {
        emit rejectClick();
    } else if (event->key() == Qt::Key_Up) {
        changeSelection(ACCEPT);
    } else if (event->key() == Qt::Key_Down) {
        if (selection_ == NONE) {
            changeSelection(ACCEPT);
        } else if (selection_ == ACCEPT) {
            changeSelection(REJECT);
        }
    }

    event->ignore();
}

bool GeneralMessageItem::sceneEvent(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Tab) {
            if (selection_ == NONE) {
                changeSelection(ACCEPT);
            } else if (selection_ == ACCEPT) {
                changeSelection(REJECT);
            } else if (selection_ == REJECT) {
                changeSelection(ACCEPT);
            }
            return true;
        }
    }

    QGraphicsItem::sceneEvent(event);
    return false;
}

void GeneralMessageItem::onHoverAccept()
{
    selection_ = ACCEPT;
}

void GeneralMessageItem::onHoverLeaveAccept()
{
    selection_ = NONE;
}

void GeneralMessageItem::onHoverReject()
{
    selection_ = REJECT;
}

void GeneralMessageItem::onHoverLeaveReject()
{
    selection_ = NONE;
}

void GeneralMessageItem::changeSelection(Selection selection)
{
    if (selection != selection_) {
        acceptButton_->unhover();
        rejectButton_->unhover();

        if (selection == ACCEPT) {
            acceptButton_->hover();
        } else if (selection == REJECT) {
            rejectButton_->hover();
        }
        selection_ = selection;
    }
}

void GeneralMessageItem::setTitleSize(int size)
{
    titleSize_ = size;
    setTitle(title_);
}

void GeneralMessageItem::updatePositions()
{
    if (icon_ != nullptr) {
        icon_->setPos(boundingRect().width()/2 - icon_->boundingRect().width()/2, 0);
    }

    if (desc_.isEmpty()) {
        setFirstItemOffsetY((titleHeight_ + 82*G_SCALE)/G_SCALE);
    } else {
        setFirstItemOffsetY((titleHeight_ + descHeight_ + 124*G_SCALE)/G_SCALE);
    }
    update();
}

} // namespace GeneralMessageWindow
