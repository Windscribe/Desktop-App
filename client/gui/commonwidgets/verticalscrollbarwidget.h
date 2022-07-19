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

    void setOpacity(double opacity);
    void setBackgroundColor(QColor color);
    void setForegroundColor(QColor color);

signals:
    void clicked();
    void moved(double posPercentY);
    void hoverEnter();
    void hoverLeave();

private slots:
    void onBarPosYChanged(const QVariant &value);

protected:
     void paintEvent(QPaintEvent *event)        override;
     void mousePressEvent(QMouseEvent *event)   override;
     void mouseReleaseEvent(QMouseEvent *event) override;
     void mouseMoveEvent(QMouseEvent *event)    override;
     void enterEvent(QEnterEvent *event) override;
     void leaveEvent(QEvent *event) override;

private:
    int width_;
    int height_;

    double curOpacity_;
    QColor curBackgroundColor_;
    QColor curForegroundColor_;

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
