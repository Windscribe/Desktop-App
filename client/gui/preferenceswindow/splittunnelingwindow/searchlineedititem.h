#pragma once

#include <QFocusEvent>
#include <QGraphicsProxyWidget>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class SearchLineEditItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit SearchLineEditItem(ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void hideButtons();
    void showButtons();
    QString text();
    void setText(QString text);

    void setFocusOnSearchBar();
    void setSelected(bool selected) override;

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void searchModeExited();
    void textChanged(QString text);
    void focusIn();
    void enterPressed();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCancelClicked();
    void onClearTextClicked();
    void onLanguageChanged();
    void onLineEditTextChanged(QString text);
    void onSearchIconOpacityChanged(const QVariant & value);

private:
    static constexpr int VERTICAL_DIVIDER_MARGIN_Y = 12;
    static constexpr int VERTICAL_DIVIDER_HEIGHT = 24;

    double searchIconOpacity_;
    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

    bool editing_;
    IconButton *clearTextButton_;
    IconButton *closeButton_;

    void exitSearchMode();

    QVariantAnimation searchIconOpacityAnimation_;

    void updatePositions();
};

}
