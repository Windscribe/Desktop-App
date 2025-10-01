#pragma once

#include <QLabel>
#include <QPushButton>

class Spinner : public QWidget
{
    Q_OBJECT
public:
    explicit Spinner(QWidget *parent);

    void start();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAnimValueChanged(const QVariant &value);

private:
    int angle_;
    bool isStarted_;
};


