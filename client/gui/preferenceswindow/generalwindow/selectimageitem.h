#ifndef SELECTIMAGEITEM_H
#define SELECTIMAGEITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class SelectImageItem : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit SelectImageItem(ScalableGraphicsObject *parent, const QString &caption);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setPath(const QString &path);
    void setCaption(const QString &caption);
    void updateScaling() override;

signals:
    void pathChanged(const QString &path);

private slots:
    void onOpenClick();

private:

    static constexpr int SELECT_IMAGE_HEIGHT = 60;
    static constexpr int SELECT_IMAGE_MARGIN_Y = 14;

    QString caption_;
    QString path_;
    QString filenameForShow_;
    IconButton *btnOpen_;

    void updatePositions();
};

} // namespace PreferencesWindow


#endif // SELECTIMAGEITEM_H
