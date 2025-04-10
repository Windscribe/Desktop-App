#pragma once

#include <QResizeEvent>
#include <QWidget>
#include <QMouseEvent>
#include "commonwidgets/iconbuttonwidget.h"

namespace GuiLocations {

class StaticIPDeviceInfo : public QWidget
{
    Q_OBJECT
public:
    explicit StaticIPDeviceInfo(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    void setDeviceName(QString deviceName);

signals:
    void addStaticIpClicked();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;

private slots:
    void onLanguageChanged();

private:
    QString deviceName_;
    bool isNameElided_;
    QRect deviceNameRect_;
    static constexpr int BOTTOM_LINE_HEIGHT = 1;
    QFont font_;

    CommonWidgets::IconButtonWidget *addButton_;

    void updatePositions();
};

}
