#ifndef LOCATIONITEMLISTWIDGET_H
#define LOCATIONITEMLISTWIDGET_H

#include <QWidget>

class LocationItemListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemListWidget(QWidget *parent = nullptr);

signals:

protected:
    virtual void paintEvent(QPaintEvent *event) override;
};

#endif // LOCATIONITEMLISTWIDGET_H
