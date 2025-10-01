#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVariantAnimation>

class ToggleButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ToggleButton(QWidget *parent);

    void toggle(bool on);

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onClicked();
    void onAnimValueChanged(const QVariant &value);

private:
    QVariantAnimation anim_;
    double animValue_;
    bool checked_;
};



