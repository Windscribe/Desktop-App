#ifndef CUSTOMSCROLLBAR_H
#define CUSTOMSCROLLBAR_H

#include <QElapsedTimer>
#include <QScrollBar>
#include <QTimer>

namespace GuiLocations {

// custom scrollbar for location dropdown list
class CustomScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit CustomScrollBar(QWidget *parent, bool bIsHidden);

    static const int SCROLL_BAR_MARGIN = 3;

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private slots:
    void onOpacityTimer();

private:
    const double HOVER_OPACITY = 1.0;
    const double UNHOVER_OPACITY = 0.5;
    const int OPACITY_DURATION = 250;

    bool bIsHidden_;
    bool isHover_;
    double curOpacity_;
    double startOpacity_;
    double endOpacity_;
    QTimer opacityTimer_;
    QElapsedTimer opacityElapsedTimer_;
};

} // namespace GuiLocations

#endif // CUSTOMSCROLLBAR_H
