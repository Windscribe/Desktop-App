#ifndef SELECTIMAGEITEM_H
#define SELECTIMAGEITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include "commongraphics/iconbutton.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class SelectImageItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit SelectImageItem(ScalableGraphicsObject *parent, const QString &caption, bool bShowDividerLine);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setPath(const QString &path);
    void updateScaling() override;

signals:
    void pathChanged(const QString &path);

private slots:
    void onOpenClick();

private:

    static constexpr int TITLE_HEIGHT = 30;
    static constexpr int BODY_HEIGHT = 43;

    QString caption_;
    QString path_;
    QString filenameForShow_;
    IconButton *btnOpen_;
    DividerLine *dividerLine_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // SELECTIMAGEITEM_H
