#ifndef USERNAMEPASSWORDENTRY_H
#define USERNAMEPASSWORDENTRY_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include <QGraphicsProxyWidget>
#include "commonwidgets/custommenulineedit.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "blockableqlineedit.h"

namespace LoginWindow {


class UsernamePasswordEntry : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit UsernamePasswordEntry(QString descriptionText, bool password, ScalableGraphicsObject * parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void clearActiveState();

    QString getText() const;

    void setError(bool error);

    void setWidth(int width);
    void setOpacityByFactor(double newOpacityFactor);

    void setClickable(bool clickable) override;

    void setFocus();
    void updateScaling() override;

signals:
    void textChanged(const QString &text);

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QGraphicsProxyWidget *userEntryProxy_;
    CommonWidgets::CustomMenuLineEdit *userEntryLine_;
    const QString userEntryLineAddSS_;

    QString descriptionText_;

    int height_;
    int width_;

    double curDescriptionOpacity_;
    double curLineEditOpacity_;

    static constexpr int DESCRIPTION_TEXT_HEIGHT = 16;
};

}

#endif // USERNAMEPASSWORDENTRY_H
