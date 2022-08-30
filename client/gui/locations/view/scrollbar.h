#pragma once

#include <QScrollBar>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include <QPropertyAnimation>

namespace gui_locations {

class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    void updateCustomStyleSheet();

public slots:
    void setValue(int value, bool bWithoutAnimation = false);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onOpacityAnimationValueChanged(const QVariant &value);

private:
    const double SCROLL_SPEED_FRACTION = 0.5;
    const int MINIMUM_DURATION = 200;
    static constexpr int kScrollAnimationDiration = 800;
    double curOpacity_;
    QVariantAnimation opacityAnimation_;
    QPropertyAnimation anim_;
    int targetValue_;

    QString customStyleSheet();
    int customPaddingWidth();
};

} // namespace gui_locations
