#pragma once

#include <QWidget>
#include <QVariantAnimation>

class UpgradeBanner : public QWidget
{
    Q_OBJECT

public:
    explicit UpgradeBanner(QWidget *parent);

    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    static constexpr qint64 TEN_GB_IN_BYTES = 10737418240;

    qint64 bytesUsed_;
    qint64 bytesMax_;
    double opacity_;
    QVariantAnimation opacityAnimation_;

    QColor getStatusColor() const;
};
