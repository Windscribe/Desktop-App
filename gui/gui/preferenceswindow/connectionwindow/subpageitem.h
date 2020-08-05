#ifndef SUBPAGEITEM_H
#define SUBPAGEITEM_H

#include <QVariantAnimation>
#include "../baseitem.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class SubPageItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SubPageItem(ScalableGraphicsObject *parent, const QString &title, bool clickable = false);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void hideOpenPopups() override;
    virtual void setArrowText(QString text);

    void updateScaling() override;

private slots:
    void onHoverEnter();
    void onHoverLeave();

    void onTextOpacityChanged(const QVariant &value);
    void onArrowOpacityChanged(const QVariant &value);

private:
    QString title_;
    QString arrowText_;

    double curTextOpacity_;
    double curArrowOpacity_;

    DividerLine *dividerLine_;

    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation arrowOpacityAnimation_;

};

} // namespace PreferencesWindow

#endif // SUBPAGEITEM_H
