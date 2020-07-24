#include "configfooterinfo.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

ConfigFooterInfo::ConfigFooterInfo(QWidget *parent) : QAbstractButton(parent)
  , pressed_(false)
  , text_(tr("Add Config Location"))
{
    font_ = *FontManager::instance().getFont(16, true);
    curTextOpacity_ = 0.5;
    curIconOpacity_ = OPACITY_UNHOVER_ICON_TEXT;

    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));
    connect(&iconOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onIconOpacityChanged(QVariant)));
}

QSize ConfigFooterInfo::sizeHint() const
{
    return QSize(WINDOW_WIDTH * G_SCALE, HEIGHT_ * G_SCALE);
}

void ConfigFooterInfo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();

    font_ = *FontManager::instance().getFont(16, true);

    painter.setOpacity(initOpacity);
    painter.fillRect(QRect(0,0,sizeHint().width(), sizeHint().height()), FontManager::instance().getCarbonBlackColor());

    painter.setOpacity(initOpacity * curTextOpacity_);
    painter.setPen(Qt::white);
    painter.setFont(font_);
    painter.drawText(QRect(WINDOW_MARGIN*G_SCALE, 14 * G_SCALE, CommonGraphics::textWidth(text_,font_), CommonGraphics::textHeight(font_)), Qt::AlignVCenter, text_);

    IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/FOLDER_ICON");
    pixmap->draw((WINDOW_WIDTH - WINDOW_MARGIN - 16) * G_SCALE, 14* G_SCALE, &painter);
}

void ConfigFooterInfo::enterEvent(QEvent *event)
{
    Q_UNUSED(event);

    setCursor(Qt::PointingHandCursor);
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void ConfigFooterInfo::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    setCursor(Qt::ArrowCursor);

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, 0.5, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_UNHOVER_ICON_TEXT, ANIMATION_SPEED_FAST);
}

void ConfigFooterInfo::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    pressed_ = true;
}

void ConfigFooterInfo::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressed_)
    {
        pressed_ = false;

        int y = geometry().y() + event->pos().y();
        QPoint pt2 = QPoint(event->pos().x(), y);

        if (geometry().contains(pt2))
        {
            emit addCustomConfigClicked();
        }
    }
}

void ConfigFooterInfo::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void ConfigFooterInfo::onIconOpacityChanged(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}
