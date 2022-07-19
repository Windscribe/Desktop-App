#include "iconwidget.h"

#include <QPainter>
#include "graphicresources/imageresourcessvg.h"

IconWidget::IconWidget(const QString &imagePath, QWidget *parent) : QWidget(parent)
  , imagePath_(imagePath)
  , curOpacity_(1)
{
}

int IconWidget::width()
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    if (p) return p->width();
    return 0;
}

int IconWidget::height()
{
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    if (p) return p->height();
    return 0;
}

void IconWidget::setOpacity(double opacity)
{
    curOpacity_ = opacity;
    update();
}

void IconWidget::setImage(const QString imagePath)
{
    imagePath_ = imagePath;
    update();
}

void IconWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();
    painter.setOpacity(curOpacity_ * initOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    p->draw(0,0,&painter);
}

void IconWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    emit hoverEnter();
}

void IconWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    emit hoverLeave();
}
