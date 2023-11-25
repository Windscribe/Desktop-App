#pragma once

#include <QLineEdit>
#include <QMouseEvent>
#include <QFocusEvent>

class BlockableQLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    BlockableQLineEdit(QWidget *parent = nullptr);

    void setClickable(bool clickable);

signals:
    void focusIn();
    void focusOut();

protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent * event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
    virtual void focusInEvent(QFocusEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    bool clickable_;
};
