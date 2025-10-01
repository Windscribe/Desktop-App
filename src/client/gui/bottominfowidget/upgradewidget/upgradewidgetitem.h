#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "commongraphics/bubblebutton.h"

namespace UpgradeWidget {


class UpgradeWidgetItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit UpgradeWidgetItem(ScalableGraphicsObject *parent);

    virtual QGraphicsObject *getGraphicsObject() { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setClickable(bool enable);
    void setWidth(int newWidth);
    QPixmap getCurrentPixmapShape();

    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax); // from 0 to 10
    void setDaysRemaining(int days);
    void setExtConfigMode(bool isExtConfigMode);

    void animateShow();
    void animateHide();

    void updateScaling() override;

signals:
    void buttonClick();

private slots:
    void onBackgroundOpacityChanged(const QVariant &value);
    void onDataRemainingTextOpacityChanged(const QVariant &value);
    void onDataRemainingIconOpacityChanged(const QVariant &value);

    void onButtonClick();
    void onLanguageChanged();

private:
    static constexpr qint64 TEN_GB_IN_BYTES = 10737418240;
    static constexpr int LEFT_TEXT_WITH_ICON_OFFSET = 24;
    static constexpr int LEFT_TEXT_NO_ICON_OFFSET = 8;
    static constexpr int VERSION_TEXT_HEIGHT = 14;

    int daysLeft_;
    qint64 bytesUsed_;
    qint64 bytesMax_;
    bool modePro_;
    bool isExtConfigMode_;

    QString curDataIconPath_;
    QString curText_;
    int curLeftTextOffset_;
    int roundedRectXRadius_;
    QColor curDataRemainingColor_;

    int width_ = 230;
    static constexpr int HEIGHT_ = 22;
    const FontDescr fontDescr_ = { 12, QFont::Normal };

    CommonGraphics::BubbleButton *textButton_;

    double curBackgroundOpacity_;
    double curDataRemainingIconOpacity_;
    double curDataRemainingTextOpacity_;

    QVariantAnimation backgroundOpacityAnimation_;
    QVariantAnimation dataRemainingTextOpacityAnimation_;
    QVariantAnimation dataRemainingIconOpacityAnimation_;

    QColor getTextColor();
    QString currentText();
    QString makeDaysLeftString(int days);

    void updateTextPos();
    void updateIconPath();
    void updateButtonText();
    void updateDisplayElements();
};

} // namespace UpgradeWindow
