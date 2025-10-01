#pragma once

#include <QAbstractButton>
#include <QVariantAnimation>
#include "commonwidgets/iconbuttonwidget.h"
#include "tooltips/tooltiptypes.h"

class ConfigFooterInfo : public QAbstractButton
{
    Q_OBJECT
public:
    explicit ConfigFooterInfo(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QString text() const;
    void setText(const QString &text);
    void updateScaling();

signals:
    void clearCustomConfigClicked();
    void addCustomConfigClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onIconOpacityChanged(const QVariant &value, int index);

private:
    struct IconButton {
        IconButton();
        double opacity;
        QVariantAnimation opacityAnimation;
        QRect rect;
        bool is_hover;
    };

    void updateDisplayText();
    void updateButtonRects();

    bool pressed_;
    QString fullText_;
    QString displayText_;
    QRect displayTextRect_;
    bool isDisplayTextRectHover_;

    static constexpr int BOTTOM_LINE_HEIGHT = 1;
    QFont font_;

    enum { ICON_CLEAR, ICON_CHOOSE, NUM_ICONS };
    IconButton iconButtons_[NUM_ICONS];
};
