#include "generalmessageitem.h"

#include <QDesktopServices>
#include <QPainter>
#include <QKeyEvent>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

namespace GeneralMessageWindow {

GeneralMessageItem::GeneralMessageItem(ScalableGraphicsObject *parent, int width, GeneralMessageWindow::Style style)
    : CommonGraphics::BasePage(parent, width), style_(style), shape_(GeneralMessageWindow::kLoginScreenShape),
    title_(""), desc_(""), titleSize_(16), acceptButton_(nullptr), rejectButton_(nullptr), tertiaryButton_(nullptr),
    showBottomPanel_(false), learnMoreUrl_(""), selection_(NONE), username_(nullptr), password_(nullptr)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(kSpacerHeight);
    setIndent(kIndent);

    checkbox_ = new Checkbox(this, tr(kRemember));
    checkbox_->setVisible(false);

    learnMoreLink_ = new CommonGraphics::TextButton(tr(kLearnMore), FontDescr(12, false), QColor(255, 255, 255), true, this);
    learnMoreLink_->setVisible(false);
    learnMoreLink_->setMarginHeight(0);
    learnMoreLink_->setUnderline(true);
    connect(learnMoreLink_, &CommonGraphics::TextButton::clicked, this, &GeneralMessageItem::onLearnMoreClick);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &GeneralMessageItem::onLanguageChanged);
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

    // bottom panel
    if (showBottomPanel_) {
        // horizontal divider
        painter->setOpacity(OPACITY_UNHOVER_DIVIDER);
        painter->drawLine(QPoint(0, boundingRect().height() - 32*G_SCALE),
                          QPoint(boundingRect().width(), boundingRect().height() - 32*G_SCALE));
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

void GeneralMessageItem::setStyle(GeneralMessageWindow::Style style)
{
    style_ = style;

    if (acceptButton_) {
        CommonGraphics::BubbleButton::Style buttonStyle = CommonGraphics::BubbleButton::kBright;
        if (style_ == GeneralMessageWindow::kDark) {
            buttonStyle = CommonGraphics::BubbleButton::kDark;
        }
        acceptButton_->setStyle(buttonStyle);
    }
    if (tertiaryButton_) {
        CommonGraphics::ListButton::Style buttonStyle = CommonGraphics::ListButton::kBright;
        if (style_ == GeneralMessageWindow::kDark) {
            buttonStyle = CommonGraphics::ListButton::kDark;
        }

        tertiaryButton_->setStyle(buttonStyle);
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

void GeneralMessageItem::setAcceptText(const QString &text, bool showRemember)
{
    if (text.isEmpty()) {
        if (acceptButton_) {
            removeItem(acceptButton_);
            acceptButton_ = nullptr;
        }
        return;
    }

    if (!acceptButton_) {
        CommonGraphics::BubbleButton::Style buttonStyle = CommonGraphics::BubbleButton::kBright;
        if (style_ == GeneralMessageWindow::kDark) {
            buttonStyle = CommonGraphics::BubbleButton::kDark;
        }

        acceptButton_ = new ButtonWithCheckbox(this, buttonStyle, text, showRemember ? tr("Remember") : "");
        connect(acceptButton_, &ButtonWithCheckbox::clicked, this, &GeneralMessageItem::acceptClick);
        connect(acceptButton_, &ButtonWithCheckbox::hoverEnter, this, &GeneralMessageItem::onHoverAccept);
        connect(acceptButton_, &ButtonWithCheckbox::hoverLeave, this, &GeneralMessageItem::onHoverLeave);
        addItem(acceptButton_);
    } else {
        acceptButton_->setShowCheckbox(showRemember ? tr("Remember") : "");
        acceptButton_->setText(text);
    }
}

void GeneralMessageItem::setTertiaryText(const QString &text)
{
    if (text.isEmpty()) {
        if (tertiaryButton_) {
            removeItem(tertiaryButton_);
            tertiaryButton_ = nullptr;
        }
        return;
    }

    if (!tertiaryButton_) {
        CommonGraphics::ListButton::Style buttonStyle = CommonGraphics::ListButton::kBright;
        if (style_ == GeneralMessageWindow::kDark) {
            buttonStyle = CommonGraphics::ListButton::kDark;
        }

        tertiaryButton_ = new CommonGraphics::ListButton(this, buttonStyle, text);
        connect(tertiaryButton_, &CommonGraphics::ListButton::clicked, this, &GeneralMessageItem::tertiaryClick);
        connect(tertiaryButton_, &CommonGraphics::ListButton::hoverEnter, this, &GeneralMessageItem::onHoverTertiary);
        connect(tertiaryButton_, &CommonGraphics::ListButton::hoverLeave, this, &GeneralMessageItem::onHoverLeave);
        addItem(tertiaryButton_);
    } else {
        tertiaryButton_->setText(text);
    }
}

void GeneralMessageItem::setRejectText(const QString &text)
{
    if (text.isEmpty()) {
        if (rejectButton_) {
            removeItem(rejectButton_);
            rejectButton_ = nullptr;
        }
        return;
    }

    if (!rejectButton_) {
        rejectButton_ = new CommonGraphics::ListButton(this, CommonGraphics::ListButton::kText, text);
        connect(rejectButton_, &CommonGraphics::ListButton::clicked, this, &GeneralMessageItem::rejectClick);
        connect(rejectButton_, &CommonGraphics::ListButton::hoverEnter, this, &GeneralMessageItem::onHoverReject);
        connect(rejectButton_, &CommonGraphics::ListButton::hoverLeave, this, &GeneralMessageItem::onHoverLeave);
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
        } else if (selection_ == TERTIARY) {
            emit tertiaryClick();
        } else if (selection_ == REJECT) {
            emit rejectClick();
        }
    } else if (event->key() == Qt::Key_Escape) {
        emit rejectClick();
    } else if (event->key() == Qt::Key_Up) {
        if (selection_ == REJECT && tertiaryButton_) {
            changeSelection(TERTIARY);
        } else if (acceptButton_) {
            changeSelection(ACCEPT);
        } else {
            changeSelection(NONE);
        }
    } else if (event->key() == Qt::Key_Down) {
        if (selection_ == NONE && acceptButton_) {
            changeSelection(ACCEPT);
        } else if (selection_ == ACCEPT && tertiaryButton_) {
            changeSelection(TERTIARY);
        } else if (rejectButton_) {
            changeSelection(REJECT);
        } else {
            changeSelection(NONE);
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

void GeneralMessageItem::onHoverReject()
{
    selection_ = REJECT;
}

void GeneralMessageItem::onHoverTertiary()
{
    selection_ = TERTIARY;
}

void GeneralMessageItem::onHoverLeave()
{
    selection_ = NONE;
}

void GeneralMessageItem::changeSelection(Selection selection)
{
    if (selection != selection_) {
        if (acceptButton_) {
            acceptButton_->unhover();
        }
        if (tertiaryButton_) {
            tertiaryButton_->unhover();
        }
        if (rejectButton_) {
            rejectButton_->unhover();
        }

        if (selection == ACCEPT && acceptButton_) {
            acceptButton_->hover();
        } else if (selection == TERTIARY && tertiaryButton_) {
            tertiaryButton_->hover();
        } else if (selection == REJECT && rejectButton_) {
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
        if (username_ != nullptr || password_ != nullptr) {
            setFirstItemOffsetY((titleHeight_ + 98*G_SCALE)/G_SCALE);
        } else {
            setFirstItemOffsetY((titleHeight_ + 82*G_SCALE)/G_SCALE);
        }
    } else {
        setFirstItemOffsetY((titleHeight_ + descHeight_ + 124*G_SCALE)/G_SCALE);
    }
    update();
}

void GeneralMessageItem::setShowBottomPanel(bool on)
{
    // to update text
    onLanguageChanged();

    if (showBottomPanel_ == on) {
        return;
    }

    showBottomPanel_ = on;

    if (on) {
        QFontMetrics fm(*FontManager::instance().getFont(14, false));
        int textWidth = fm.horizontalAdvance(tr(kLearnMore));
        checkbox_->setPos(16*G_SCALE, fullHeight() + 40*G_SCALE);
        learnMoreLink_->setPos(boundingRect().width() - 16*G_SCALE - textWidth, fullHeight() + 40*G_SCALE);
    }
    checkbox_->setVisible(on);
    learnMoreLink_->setVisible(on);

    setEndSpacing(on ? 56 : 0);
    update();
}

void GeneralMessageItem::setLearnMoreUrl(const QString &url)
{
    learnMoreUrl_ = url;
}

bool GeneralMessageItem::isRememberChecked()
{
    return (acceptButton_->isCheckboxChecked() || checkbox_->isChecked());
}

void GeneralMessageItem::setUsername(const QString &username)
{
    username_->setLineEditText(username);
}

void GeneralMessageItem::onLanguageChanged()
{
    checkbox_->setText(tr(kRemember));
    learnMoreLink_->setText(tr(kLearnMore));
}

void GeneralMessageItem::onLearnMoreClick()
{
    QDesktopServices::openUrl(QUrl(learnMoreUrl_));
}

void GeneralMessageItem::setShowUsername(bool on)
{
    if (on) {
        if (username_ == nullptr) {
            username_ = new CredentialLineEdit(this, tr("Username"));
            connect(username_, &CredentialLineEdit::editingFinished, this, &GeneralMessageItem::onCredentialEditingFinished);
            addItem(username_, 0);
        }
    } else {
        removeItem(username_);
        username_ = nullptr;
    }
    updatePositions();
}

void GeneralMessageItem::setShowPassword(bool on)
{
    if (on) {
        if (password_ == nullptr) {
            password_ = new CredentialLineEdit(this, tr("Password"), true);
            connect(password_, &CredentialLineEdit::editingFinished, this, &GeneralMessageItem::onCredentialEditingFinished);
            addItem(password_, username_ ? 1 : 0);
        }
    } else {
        removeItem(password_);
        password_ = nullptr;
    }
    updatePositions();
}

QString GeneralMessageItem::username() const
{
    if (username_ == nullptr) {
        return QString();
    }
    return username_->text();
}

QString GeneralMessageItem::password() const
{
    if (password_ == nullptr) {
        return QString();
    }
    return password_->text();
}

void GeneralMessageItem::clear()
{
    if (username_) {
        username_->clear();
    }
    if (password_) {
        password_->clear();
    }

    selection_ = NONE;
}

void GeneralMessageItem::focusInEvent(QFocusEvent *event)
{
    if (username_ && username_->text().isEmpty()) {
        username_->setFocus();
    } else if (password_) {
        password_->setFocus();
    }
}

void GeneralMessageItem::onCredentialEditingFinished()
{
    CredentialLineEdit *source = static_cast<CredentialLineEdit *>(sender());
    if (source == username_) {
        if (password_) {
            password_->setFocus();
        } else {
            changeSelection(ACCEPT);
        }
    } else if (source == password_) {
        changeSelection(ACCEPT);
    }
}

} // namespace GeneralMessageWindow
