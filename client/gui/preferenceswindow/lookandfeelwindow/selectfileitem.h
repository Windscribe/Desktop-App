#pragma once

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include "commongraphics/baseitem.h"
#include "commongraphics/texticonbutton.h"

namespace PreferencesWindow {

class SelectFileItem : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit SelectFileItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;
    void updateScaling() override;

    void setPath(const QString &path);
    QString path() const { return path_; }

    void setDialogText(const QString &title, const QString &filter);
    void setMaxWidth(int maxWidth);

signals:
    void pathChanged(const QString &path);

private slots:
    void onClick();
    void onLanguageChanged();

private:
    static constexpr int kSelectImageHeight = 12;

    QString path_;
    QString filename_;
    CommonGraphics::TextIconButton *button_;

    QString dialogTitle_;
    QString dialogFilter_;

    int width_;
    int maxWidth_;

    void updatePositions();
};

} // namespace PreferencesWindow
