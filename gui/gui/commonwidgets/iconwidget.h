#ifndef ICONWIDGET_H
#define ICONWIDGET_H

#include <QWidget>
#include <QVariantAnimation>

class IconWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IconWidget(const QString &imagePath, QWidget *parent = nullptr);

    int width();
    int height();

    void setOpacity(double opacity);
    void updateScaling();

signals:
    void hoverEnter();
    void hoverLeave();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString imagePath_;
    double curOpacity_;

};

#endif // ICONWIDGET_H
