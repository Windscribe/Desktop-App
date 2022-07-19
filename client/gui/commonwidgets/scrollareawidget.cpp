#include "scrollareawidget.h"

#include <QWheelEvent>
#include <QTimer>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

#include <QPainter>

ScrollAreaWidget::ScrollAreaWidget(int width, int height, QWidget *parent) : QWidget(parent)
    , width_(width)
    , height_(height)
    , lastWidgetPos_(0)
    , centralWidget_(nullptr)
{   
    hLayout_ = new QHBoxLayout(this);
    hLayout_->setContentsMargins(0, 0, 0, 0);
    hLayout_->setSpacing(0);

    const int kMinimumScrollBarWidth = 5;
    scrollBar_ = new VerticalScrollBarWidget(kMinimumScrollBarWidth, height - SCROLL_BAR_GAP*2);
    scrollBar_->setMinimumWidth(kMinimumScrollBarWidth);
    connect(scrollBar_, SIGNAL(moved(double)), SLOT(onScrollBarMoved(double)));

    connect(&itemPosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onItemPosYChanged(QVariant)));
}

QSize ScrollAreaWidget::sizeHint() const
{
    return QSize(width_ * G_SCALE, height_ * G_SCALE);
}

void ScrollAreaWidget::setCentralWidget(IScrollableWidget *widget)
{
    if (centralWidget_ != nullptr)
    {
        hLayout_->removeWidget(dynamic_cast<QWidget *>(centralWidget_));
        centralWidget_->getWidget()->deleteLater();
        disconnect(dynamic_cast<QObject*>(centralWidget_), SIGNAL(shiftPos(int)), this, SLOT(onWidgetShift(int)));
        disconnect(dynamic_cast<QObject*>(centralWidget_), SIGNAL(recenterWidget()), this, SLOT(onRecenterWidget()));

        hLayout_->removeWidget(scrollBar_);
    }

    centralWidget_ = widget;
    lastWidgetPos_ = widget->getWidget()->pos().y() / G_SCALE;


    hLayout_->addWidget(dynamic_cast<QWidget *>(widget));
    hLayout_->addWidget(scrollBar_);

    widget->getWidget()->show();
    scrollBar_->show();

    connect(dynamic_cast<QObject*>(widget), SIGNAL(shiftPos(int)), SLOT(onWidgetShift(int)));
    connect(dynamic_cast<QObject*>(widget), SIGNAL(recenterWidget()), SLOT(onRecenterWidget()));

    updateScrollBarHeightAndPos();
}

void ScrollAreaWidget::setWidth(int width)
{
    width_ = width;
    updateGeometry();
}

void ScrollAreaWidget::setHeight(int height)
{
    int change = abs(height_ - height);

    height_ = height;
    updateGeometry();

    centralWidget_->setViewportHeight(height);

    updateScrollBarHeightAndPos();

    // if top region occluded then bring item down with window
    if (centralWidget_->getWidget()->pos().y() < 0)
    {
        const int newY = centralWidget_->getWidget()->pos().y() - change * G_SCALE;
        moveWidgetPosY(newY);

    }
}

void ScrollAreaWidget::setScrollBarColor(QColor color)
{
    scrollBar_->setForegroundColor(color);
}

void ScrollAreaWidget::updateSizing()
{
    updateScrollBarHeightAndPos();

    double stepPercent = static_cast<double>(centralWidget_->stepSize()) / centralWidget_->sizeHint().height();
    scrollBar_->setStepSizePercent(stepPercent);
}

int ScrollAreaWidget::discretePosY(int newY)
{
    if (newY < centralWidget_->getWidget()->pos().y()) // scrolling down
    {
        int i = 1;
        int curStep = centralWidget_->getWidget()->pos().y() - centralWidget_->stepSize()*i;
        while (newY > curStep)
        {
            curStep -= centralWidget_->stepSize()*i;
            i++;
        }
        newY = curStep;
    }
    else if (newY >= centralWidget_->getWidget()->pos().y())
    {
        int i = 1;
        int curStep = centralWidget_->getWidget()->pos().y() + centralWidget_->stepSize()*i;
        while (newY < curStep)
        {
            curStep += centralWidget_->stepSize()*i;
            i++;
        }
        newY = curStep;
    }

    return newY;
}

void ScrollAreaWidget::wheelEvent(QWheelEvent *event)
{
    int mouseSpeedFactor = 2;
    int newY = static_cast<int>(centralWidget_->getWidget()->pos().y() + event->angleDelta().y()/mouseSpeedFactor);

    newY = discretePosY(newY);

    moveWidgetPosY(newY);
}

void ScrollAreaWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // QPainter painter(this);
    // painter.fillRect(QRect(0,0, sizeHint().width(), sizeHint().height()), Qt::red);
}

void ScrollAreaWidget::onScrollBarMoved(double posPercentY)
{
    int newPosY = - static_cast<int>(centralWidget_->sizeHint().height() * posPercentY);
    moveWidgetPosY(newPosY);
}

void ScrollAreaWidget::onItemPosYChanged(const QVariant &value)
{
    moveWidgetPosY(value.toInt());
}

void ScrollAreaWidget::onWidgetShift(int newPos)
{
    if (newPos > 0) newPos = 0;
    if (newPos < lowestY()) newPos = lowestY();

    startAnAnimation(itemPosAnimation_, static_cast<int>(centralWidget_->getWidget()->pos().y()), newPos, 150);
}

void ScrollAreaWidget::onRecenterWidget()
{
    // Resizing the widget height causes an unwanted position reset for some reason.
    // This line undoes the unwanted reset
    // Timer because otherwise the fix will occur before the reset
    QTimer::singleShot(1, [this]() {
        moveWidgetPosY(lastWidgetPos_ * G_SCALE);
        updateSizing();
    });
}

void ScrollAreaWidget::updateScrollBarHeightAndPos()
{
    double sf = screenFraction();

    if (sf < 1)
    {
        scrollBar_->show();
    }
    else
    {
        scrollBar_->hide();
    }

    const int scrollBarHeight = height_ - SCROLL_BAR_GAP*2.5;
    scrollBar_->setHeight(scrollBarHeight * G_SCALE);
    scrollBar_->setBarPortion(sf);
    updateScrollBarPos();
}

void ScrollAreaWidget::updateScrollBarPos()
{
    double posYPercent = static_cast<double>(centralWidget_->getWidget()->pos().y()) / centralWidget_->sizeHint().height();
    scrollBar_->moveBarToPercentPos(posYPercent);
}

double ScrollAreaWidget::screenFraction()
{
    const double screenHeight = sizeHint().height();
    double screenFraction = 1;

    const int widgetHeight = centralWidget_->sizeHint().height();
    if (widgetHeight > screenHeight) screenFraction = (screenHeight / widgetHeight);

    return screenFraction;
}

void ScrollAreaWidget::setLayoutConstraint(QLayout::SizeConstraint constraint)
{
    hLayout_->setSizeConstraint(constraint);
}

int ScrollAreaWidget::lowestY()
{
    int lowest = height_ * G_SCALE - static_cast<int>(centralWidget_->sizeHint().height());
    if (lowest > 0) lowest = 0;
    return lowest;
}

void ScrollAreaWidget::moveWidgetPosY(int newY)
{
    if (newY > 0) newY = 0;
    if (newY < lowestY()) newY = lowestY();

    if (centralWidget_->getWidget()->pos().y() != newY)
    {
        // For some reason gui (and other callers) don't detect centralWidget pos change right away (only the first time)
        // timer provides enough time (only viisble rarely)
        QTimer::singleShot(15, [=] ()
        {
            centralWidget_->getWidget()->move(centralWidget_->getWidget()->pos().x(), newY);
            lastWidgetPos_ = centralWidget_->getWidget()->pos().y() / G_SCALE;

            updateScrollBarPos();
        });
    }
}
