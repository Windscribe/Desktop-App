#pragma once

#include <QScrollBar>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include <QPropertyAnimation>

namespace CommonWidgets {

// Scroll bar with the custom style and and support for smooth scrolling animation
class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    void updateCustomStyleSheet();

public slots:
    void setValueWithAnimation(int value);
    void setValueWithoutAnimation(int value);
    void scrollDx(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    static constexpr int kScrollAnimationDiration = 800;
    QVariantAnimation scrollAnimation_;
    int targetValue_;

    QString customStyleSheet();
    int customPaddingWidth();
};

} // namespace CommonWidgets
