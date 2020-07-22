#include "scrollablemessage.h"

#include <QPainter>
#include <QTextDocument>
#include <QTextBrowser>
#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QLayout>
#include <QCursor>
#include "GraphicResources/fontmanager.h"
#include "CommonGraphics/commongraphics.h"
#include "dpiscalemanager.h"

#include <QFontMetrics>

#include <QDebug>

const int OFFSET_FOR_VIEW = 2;

ScrollableMessage::ScrollableMessage(int width, int height, QGraphicsObject *parent) : ScalableGraphicsObject(parent)
{
    height_ = height;
    width_ = width;

    textBrowser_ = new QTextBrowser();
    textBrowser_->setAttribute(Qt::WA_OpaquePaintEvent, false);
    textBrowser_->setOpenExternalLinks(true);
    textBrowser_->setFrameStyle(QFrame::NoFrame);
    textBrowser_->installEventFilter(this);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(textBrowser_);

    updatePositions();
}

QRectF ScrollableMessage::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void ScrollableMessage::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}


void ScrollableMessage::setMessage(const QString &strMessage)
{
    QTextDocument *doc = textBrowser_->document();
    doc->setTextWidth((width_-OFFSET_FOR_VIEW)*G_SCALE);
    doc->setHtml(strMessage);

    update();
}

void ScrollableMessage::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void ScrollableMessage::updatePositions()
{

    // TODO: REFACTOR THIS CSS BS
    QString browserSheet = QString("QScrollBar:vertical { margin: 0px 0px 0px 0px; border: none ; background: rgba(255, 255, 255, 20%); width: %1px; }"   // background
                                   "QScrollBar::handle:vertical { background: rgb(255, 255, 255); color:  rgb(255, 255, 255);"                      // foreground
                                                                                "border-width: 1px; border-style: solid; }"
                                                                                "QScrollBar::add-line:vertical { border: none; background: none; }" // fix top and bottom notches not right color
                                                                                "QScrollBar::sub-line:vertical { border: none; background: none; }"
                                   "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"                             // remove checkerboard
                                   "QTextBrowser {margin-left:0px; background-color: rgba(0, 0, 0, 0%); color: rgb(255, 255, 255); %2; }"
                                   )
                                   .arg(2).arg( FontManager::instance().getFontStyleSheet(12, false));
    textBrowser_->setStyleSheet(browserSheet);
    textBrowser_->setReadOnly(true);
    textBrowser_->setFixedWidth(width_ *G_SCALE);
    textBrowser_->setFixedHeight(height_*G_SCALE);

    QString docSheet = "a { text-decoration: underline; color: rgb(87, 126, 215) };";
    QTextDocument * doc = textBrowser_->document();
    doc->setDefaultStyleSheet(docSheet);
    doc->setTextWidth((width_-OFFSET_FOR_VIEW)*G_SCALE);

}

bool ScrollableMessage::eventFilter(QObject *watching, QEvent *event)
{
    // QTBUG-3259: QGraphicsProxyWidget doesn't listen to widget's cursor changes.
    // Doesn't seem it's gonna be fixed, so propagate cursor changes manually.
    if (watching == textBrowser_ && event->type() == QEvent::CursorChange)
        proxyWidget_->setCursor(textBrowser_->viewport()->cursor());

    return false;
}
