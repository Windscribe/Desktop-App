#include "messageitem.h"

#include <QPainter>
#include <QTextDocument>
#include <QTextBrowser>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "newsfeedconst.h"

namespace NewsFeedWindow {

MessageItem::MessageItem(QGraphicsObject *parent, int width, QString msg) : ScalableGraphicsObject(parent), msg_(msg)
{
    width_ = width;

    textBrowser_ = new QTextBrowser();
    textBrowser_->setAttribute(Qt::WA_OpaquePaintEvent, false);
    textBrowser_->setOpenExternalLinks(true);
    textBrowser_->setFrameStyle(QFrame::NoFrame);
    textBrowser_->setReadOnly(true);
    textBrowser_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textBrowser_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textBrowser_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    textBrowser_->installEventFilter(this);
    textBrowser_->setContextMenuPolicy(Qt::NoContextMenu);

    QTextDocument *doc = textBrowser_->document();
    doc->setHtml(msg_);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(textBrowser_);

    updatePositions();
}

QRectF MessageItem::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, textBrowser_->height());
}

void MessageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void MessageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void MessageItem::updatePositions()
{
    QString browserSheet = QString("QTextBrowser { margin: 0; background-color: transparent; %1; }").arg(FontManager::instance().getFontStyleSheet(14, QFont::Normal));
    textBrowser_->setStyleSheet(browserSheet);

    QString docSheet = "a.ncta { color: #17e9ad; text-decoration: underline; }"
                       "p { color: #99ffffff; }";

    QTextDocument *doc = textBrowser_->document();
    doc->setDefaultStyleSheet(docSheet);
    doc->setDocumentMargin(TEXT_MARGIN*G_SCALE);
    doc->setHtml(msg_);

    textBrowser_->setFixedWidth(width_*G_SCALE);
    textBrowser_->setFixedHeight(textBrowser_->document()->size().height());
}

bool MessageItem::eventFilter(QObject *watching, QEvent *event)
{
    // QTBUG-3259: QGraphicsProxyWidget doesn't listen to widget's cursor changes.
    // Doesn't seem it's gonna be fixed, so propagate cursor changes manually.
    if (watching == textBrowser_ && event->type() == QEvent::CursorChange)
    {
        proxyWidget_->setCursor(textBrowser_->viewport()->cursor());
    }

    if (watching == textBrowser_ && event->type() == QEvent::Wheel)
    {
        event->ignore();
        return true;
    }

    return false;
}

} // namespace NewsFeedWindow