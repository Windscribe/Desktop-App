#pragma once

#include <QWidget>
#include <QTimer>

// This class can be used to add DPI scale awareness to dialogs. These windows will get a resize
// event, which triggers the |updateScaling| function that can be overriden.
// It also attempts to fix a Qt DPI scaling bug on Windows, where windows receive the WM_DPICHANGED
// message with an updated geometry, assign it, but do not perform any internal updates. As a
// result, they don't get resize events. Qt postpones the update, because it expects a resize event
// to be issued later, on screen change detection. But if we change DPI manually, not by dragging
// a window to another screen, but in Display Settings, there will be no resize event emitted. This
// simple fix uses a timer to check if the resize event wasn't sent, and to send it manually.

class DPIScaleAwareWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DPIScaleAwareWidget(QWidget *parent = nullptr);

    double currentScale() const { return currentScale_; }
    void checkForAutoResize_win();

protected:
    virtual void updateScaling() {}
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void performAutoResize_win();

private:
    double detectCurrentScale() const;
    double currentScale_;
    int currentWidth_;
    int currentHeight_;
#if defined(Q_OS_WIN)
    QTimer autoResizeTimer_;
#endif
};
