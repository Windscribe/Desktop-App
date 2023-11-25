#include "dpiscaleawarewidget.h"
#include "dpiscalemanager.h"

#include <QGuiApplication>
#include <QResizeEvent>

DPIScaleAwareWidget::DPIScaleAwareWidget(QWidget *parent)
    : QWidget(parent), currentScale_(G_SCALE), currentWidth_(width()), currentHeight_(height())
{
#if defined(Q_OS_WIN)
    connect(&autoResizeTimer_, &QTimer::timeout, this, &DPIScaleAwareWidget::performAutoResize_win);
    autoResizeTimer_.setInterval(100);
    autoResizeTimer_.setSingleShot(true);
#endif  // Q_OS_WIN
}

void DPIScaleAwareWidget::resizeEvent(QResizeEvent *event)
{
#if defined(Q_OS_WIN)
    autoResizeTimer_.stop();
#endif
    const double scale = detectCurrentScale();
    currentWidth_ = event->size().width() / scale;
    currentHeight_ = event->size().height() / scale;

    if (currentScale_ != scale) {
        currentScale_ = scale;
        updateScaling();
    }
}

void DPIScaleAwareWidget::checkForAutoResize_win()
{
#if defined(Q_OS_WIN)
    autoResizeTimer_.start();
#endif
}

void DPIScaleAwareWidget::performAutoResize_win()
{
#if defined(Q_OS_WIN)
    const double scale = detectCurrentScale();
    resize(QSize(currentWidth_ * scale, currentHeight_ * scale));
#endif  // Q_OS_WIN
}

double DPIScaleAwareWidget::detectCurrentScale() const
{
    const QPoint pt(geometry().x() + geometry().width() / 2, geometry().y());
    const auto *screen = QGuiApplication::screenAt(pt);
    return screen ? DpiScaleManager::instance().scaleOfScreen(screen) : 1.0;
}
