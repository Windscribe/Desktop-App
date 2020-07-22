#ifndef VERTICALSCROLLBARWIDGET_H
#define VERTICALSCROLLBARWIDGET_H

#include <QWidget>
#include <QVariantAnimation>

class VerticalScrollBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VerticalScrollBarWidget(int width, int height, QWidget *parent = nullptr);

    void setHeight(int height);
    void setBarPortion(double portion);
    void setStepSizePercent(double stepSizePrecent);
    void moveBarToPercentPos(double posPercentY);

    void setBackgroundColor(QColor color);
    void setForegroundColor(QColor color);

signals:
    void clicked();
    void moved(double posPercentY);

private slots:
    void onBarPosYChanged(const QVariant &value);

protected:
     void paintEvent(QPaintEvent *event)        override;
     void mousePressEvent(QMouseEvent *event)   override;
     void mouseReleaseEvent(QMouseEvent *event) override;
     void mouseMoveEvent(QMouseEvent *event)    override;

private:
    int width_; // clickable width of bar
    int height_;

    QColor curBackgroundColor_;
    QColor curForegroundColor_;

    int drawWidth_; // visible width of bar
    int drawWidthPosY_;

    int curBarPosY_;
    int curBarHeight_;

    double barPortion_;
    double stepSize_;

    bool inBarRegion(int y);

    void updateBarHeight();

    bool pressed_;
    int mouseOnClickY_;

    QVariantAnimation barPosYAnimation_;
};

#endif // VERTICALSCROLLBARWIDGET_H
