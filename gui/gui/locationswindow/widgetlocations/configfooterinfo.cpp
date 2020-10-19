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
  , fullText_(tr("Set Config Location"))
  , displayText_(fullText_)
  , font_(*FontManager::instance().getFont(16, true))
  , curTextOpacity_(0.5)
  , curIconOpacity_(OPACITY_UNHOVER_ICON_TEXT)
{
    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));
    connect(&iconOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onIconOpacityChanged(QVariant)));
}

QSize ConfigFooterInfo::sizeHint() const
{
    return QSize(WINDOW_WIDTH * G_SCALE, HEIGHT_ * G_SCALE);
}

void ConfigFooterInfo::setText(const QString &text)
{
    fullText_ = text;
    updateDisplayText();
    update();
}

void ConfigFooterInfo::updateDisplayText()
{
    font_ = *FontManager::instance().getFont(14, false);
    displayText_ = CommonGraphics::truncateText(fullText_, font_,
        width() - (WINDOW_MARGIN * 4 + 16) * G_SCALE);
}

void ConfigFooterInfo::updateScaling()
{
    updateDisplayText();
}

void ConfigFooterInfo::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    qreal initOpacity = painter.opacity();

    painter.setOpacity(initOpacity);
    painter.fillRect(QRect(0,0,sizeHint().width(), sizeHint().height()), FontManager::instance().getCarbonBlackColor());

    if (!displayText_.isEmpty()) {
        font_ = *FontManager::instance().getFont(14, false);
        painter.setOpacity(initOpacity * curTextOpacity_);
        painter.setPen(Qt::white);
        painter.setFont(font_);
        painter.drawText(QRect(WINDOW_MARGIN*G_SCALE, 14 * G_SCALE,
            CommonGraphics::textWidth(displayText_, font_),
            CommonGraphics::textHeight(font_)), Qt::AlignVCenter, displayText_);
    }

    IndependentPixmap *pixmap = ImageResourcesSvg::instance().getIndependentPixmap("locations/EDIT_ICON");
    pixmap->draw((WINDOW_WIDTH - WINDOW_MARGIN - 16) * G_SCALE, 14* G_SCALE, &painter);
}

void ConfigFooterInfo::enterEvent(QEvent *event)
{
    Q_UNUSED(event);

    if (fullText_ != displayText_) {
        QPoint pt = mapToGlobal(QPoint(WINDOW_MARGIN * G_SCALE
            + CommonGraphics::textWidth(displayText_, font_) / 2, 14 * G_SCALE));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_CONFIG_FOOTER);
        ti.x = pt.x();
        ti.y = pt.y();
        ti.title = fullText_;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        emit showTooltip(ti);
    }

    setCursor(Qt::PointingHandCursor);
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
}

void ConfigFooterInfo::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    emit hideTooltip(TOOLTIP_ID_CONFIG_FOOTER);
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
