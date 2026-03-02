#pragma once

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

    explicit UsernamePasswordEntry(const QString &text, bool password, ScalableGraphicsObject * parent = nullptr, bool showDescription = true);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void clearActiveState();

    QString getText() const;
    void setText(const QString &text);
    void setDescription(const QString &text);
    void setPlaceholderText(const QString &placeholderText);

    void setError(bool error);

    void setWidth(int width);
    void setOpacityByFactor(double newOpacityFactor);

    void setClickable(bool clickable) override;

    void setFocus();
    void updateScaling() override;

    // Icon configuration - supports 0, 1, or 2 icons
    void setCustomIcon1(const QString &iconUrl);
    void setCustomIcon2(const QString &iconUrl);

    void setFont(const QFont &font);

signals:
    void textChanged(const QString &text);
    void icon1Clicked();
    void icon2Clicked();

private slots:
    void onTextChanged(const QString &text);

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QGraphicsProxyWidget *userEntryProxy_;
    CommonWidgets::CustomMenuLineEdit *userEntryLine_;
    const QString userEntryLineAddSS_;

    QFont font_;

    QString descriptionText_;
    bool showDescription_;

    int height_;
    int width_;

    double curDescriptionOpacity_;
    double curLineEditOpacity_;

    static constexpr int DESCRIPTION_TEXT_HEIGHT = 16;
    static constexpr int PLACEHOLDER_FONT_SIZE = 12;
    static constexpr int DEFAULT_FONT_SIZE = 14;

    void updateFontSize();
};

}
