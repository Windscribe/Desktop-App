#pragma once

#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>

class HoverButton : public QPushButton
{
    Q_OBJECT
public:
    // For text-only buttons
    explicit HoverButton(QWidget *parent);
    // For single-image buttons which brighten on hover
    explicit HoverButton(QWidget *parent, const QString &imagePath);
    // For two-image buttons which switch on hover
    explicit HoverButton(QWidget *parent, const QString &imagePath, const QString &hoverImagePath);

    void setText(const QString &text);
    void setTextSize(int size);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QLabel *icon_;
    QLabel *hoverIcon_;
    QLabel *text_;
    int textSize_;
    double hoverOpacity_;

    QPropertyAnimation anim;

    void init();
};
