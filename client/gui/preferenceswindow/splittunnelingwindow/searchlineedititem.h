#ifndef SEARCHLINEEDITITEM_H
#define SEARCHLINEEDITITEM_H

#include <QFocusEvent>
#include <QGraphicsProxyWidget>
#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class SearchLineEditItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SearchLineEditItem(ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void hideButtons();
    void showButtons();
    QString getText();

    void setFocusOnSearchBar();
    void setSelected(bool selected) override;

    void updateScaling() override;
    bool hasItemWithFocus() override;

signals:
    void searchModeExited();
    void textChanged(QString text);
    void enterPressed();
    void focusIn();

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
    double searchIconOpacity_;
    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

    bool editing_;
    IconButton *clearTextButton_;
    IconButton *closeButton_;
    DividerLine *line_;

    void exitSearchMode();

    QVariantAnimation searchIconOpacityAnimation_;

    void updatePositions();
};

}

#endif // SEARCHLINEEDITITEM_H
