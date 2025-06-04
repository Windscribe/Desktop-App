#pragma once

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
    // This is the icon on the left side of the link text, if any
    void setIcon(QSharedPointer<IndependentPixmap> icon);
    // This is the icon on the far right (e.g. external link or subpage link arrow)
    void setLinkIcon(QSharedPointer<IndependentPixmap> icon);
    void setInProgress(bool inProgress);

    void setDescription(const QString &description, const QString &url = "");

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onOpenUrl();
    void onHoverEnter();
    void onHoverLeave();

    void onTextOpacityChanged(const QVariant &value);
    void onArrowOpacityChanged(const QVariant &value);
    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();
    void onInfoIconClicked();

private:
    IconButton *button_;
    QString title_;
    QString url_;
    QString linkText_;
    LinkType type_;
    QSharedPointer<IndependentPixmap> icon_;
    QSharedPointer<IndependentPixmap> linkIcon_;
    bool inProgress_;
    int spinnerRotation_;

    double curTextOpacity_;
    double curArrowOpacity_;

    QVariantAnimation textOpacityAnimation_;
    QVariantAnimation arrowOpacityAnimation_;
    QVariantAnimation spinnerAnimation_;

    bool isTitleElided_ = false;
    QRectF titleRect_;

    QString desc_;
    QString descUrl_;
    IconButton *infoIcon_;
    int descHeight_ = 0;
    int descRightMargin_ = 0;
    int yOffset_;

    void updatePositions();
};

} // namespace PreferencesWindow
