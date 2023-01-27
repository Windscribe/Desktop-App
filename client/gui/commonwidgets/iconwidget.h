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

    // May not currently support image-size changes
    void setImage(const QString imagePath);

signals:
    void hoverEnter();
    void hoverLeave();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString imagePath_;
    double curOpacity_;

};

#endif // ICONWIDGET_H
