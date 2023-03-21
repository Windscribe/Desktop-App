#ifndef LINKITEM_H
#define LINKITEM_H

#include <QLineEdit>
#include <QSharedPointer>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "graphicresources/independentpixmap.h"

namespace PreferencesWindow {

class LinkItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    enum LinkType {
        TEXT_ONLY,
        BLANK_LINK,
        SUBPAGE_LINK,
        EXTERNAL_LINK,
    };

    explicit LinkItem(ScalableGraphicsObject *parent, LinkType type, const QString &title = "", const QString &url = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void hideOpenPopups() override;
    void updateScaling() override;

    QString title();
    void setTitle(const QString &title);
    QString linkText();
    void setLinkText(const QString &text);
    void setUrl(const QString &url);
    void setIcon(QSharedPointer<IndependentPixmap> icon);

private slots:
    void onOpenUrl();
    void onHoverEnter();
    void onHoverLeave();

    void onTextOpacityChanged(const QVariant &value);
    void onArrowOpacityChanged(const QVariant &value);

private:
    IconButton *button_;
    QString title_;
    QString url_;
    QString linkText_;
    LinkType type_;
    QSharedPointer<IndependentPixmap> icon_;

    double curTextOpacity_;
    double curArrowOpacity_;

    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation arrowOpacityAnimation_;
};

} // namespace PreferencesWindow

#endif // LINKITEM_H
