#ifndef STATICIPDEVICEINFO_H
#define STATICIPDEVICEINFO_H

#include <QWidget>
#include <QVariantAnimation>

namespace GuiLocations {

class StaticIPDeviceInfo : public QWidget
{
    Q_OBJECT
public:
    explicit StaticIPDeviceInfo(QWidget *parent = nullptr);
    QSize sizeHint() const;
    void setDeviceName(QString deviceName);
signals:
    void addStaticIpClicked();

protected:
    virtual void paintEvent(QPaintEvent *event);

    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void onTextOpacityChange(const QVariant &value);
    void onIconOpacityChange(const QVariant &value);

private:
    bool pressed_;
    QString iconPath_;

    int addStaticTextWidth_;
    int addStaticTextHeight_;

    QString deviceName_;
    static constexpr int BOTTOM_LINE_HEIGHT = 1;
    QFont font_;

    double curTextOpacity_;
    double curIconOpacity_;
    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation iconOpacityAnimation_;

};

}
#endif // STATICIPDEVICEINFO_H
