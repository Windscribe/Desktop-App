#pragma once

#include <QObject>
#include <QRect>
#include <QFont>

class LightWidget : public QObject
{
    Q_OBJECT
public:
    LightWidget(QObject *parent = nullptr);
    ~LightWidget();

    void setRect(const QRect &r);
    const QRect &rect();

    void setHovering(bool hovering);
    bool hovering();

    void setText(const QString &text);
    const QString &text();

    void setFont(const QFont &font);
    const QFont &font();

    // only use for text
    int truncatedTextWidth(int maxWidth);
    int textWidth();
    int textHeight();

    void setIcon(const QString &icon);
    const QString &icon();

    void setOpacity(double opacity);
    double opacity();

signals:
    void hoveringChanged(bool hovering);

private:
    QFont font_;
    QRect rect_;
    QString text_;
    QString icon_;
    bool hovering_;
    double opacity_;

};
