#include "textlinkitem.h"

#include <QDesktopServices>
#include <QGraphicsSceneMouseEvent>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"

namespace EmergencyConnectWindow {

TextLinkItem::TextLinkItem(QGraphicsObject *parent)
    : QTextBrowser(), linkOpacity_(OPACITY_UNHOVER_TEXT), linkBegin_(0), linkEnd_(0)
{
    setMouseTracking(true);

    setStyleSheet(QString("QTextBrowser { background: transparent; color: rgba(255, 255, 255, 128); }"));
    setTextInteractionFlags(Qt::NoTextInteraction);

    proxy_ = new QGraphicsProxyWidget(parent);
    proxy_->setWidget(this);

    connect(&linkOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextLinkItem::onLinkOpacityChanged);
    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &TextLinkItem::onLanguageChanged);

    onLanguageChanged();
}

void TextLinkItem::mouseMoveEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->position().toPoint());
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.selectionStart() >= linkBegin_ && cursor.selectionEnd() <= linkEnd_) {
        startAnAnimation(linkOpacityAnimation_, linkOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    } else {
        startAnAnimation(linkOpacityAnimation_, linkOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    }
}

void TextLinkItem::mousePressEvent(QMouseEvent *event)
{
    QTextCursor cursor = cursorForPosition(event->position().toPoint());
    cursor.select(QTextCursor::WordUnderCursor);
    if (cursor.selectionStart() >= linkBegin_ && cursor.selectionEnd() <= linkEnd_) {
        QDesktopServices::openUrl(QUrl("https://windscribe.com"));
    }
}

void TextLinkItem::onLanguageChanged()
{
    onLinkOpacityChanged(linkOpacity_);

    setHtml(tr("Access for %1 Only").arg("<a href=\"https://windscribe.com\"><font color=\"#%1ffffff\">Windscribe.com</font></a>").arg(QString::number(int(linkOpacity_*255), 16)));
    QString rawText = toPlainText();
    linkBegin_ = rawText.indexOf("Windscribe.com");
    linkEnd_ = rawText.indexOf("Windscribe.com") + 14;
    setAlignment(Qt::AlignCenter);
}

void TextLinkItem::onLinkOpacityChanged(const QVariant &value)
{
    linkOpacity_ = value.toDouble();
    setHtml(tr("Access for %1 Only").arg("<a href=\"https://windscribe.com\"><font color=\"#%1ffffff\">Windscribe.com</font></a>").arg(QString::number(int(linkOpacity_*255), 16)));
    setAlignment(Qt::AlignCenter);
}

void TextLinkItem::updateScaling()
{
    setFixedWidth(WINDOW_WIDTH*G_SCALE);
}

} // namespace EmergencyConnectWindow
