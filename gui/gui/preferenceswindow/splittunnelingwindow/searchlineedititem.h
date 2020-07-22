#ifndef SEARCHLINEEDITITEM_H
#define SEARCHLINEEDITITEM_H

#include <QFocusEvent>
#include <QGraphicsProxyWidget>
#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"
#include "CommonGraphics/custommenulineedit.h"

namespace PreferencesWindow {

class SearchLineEditItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SearchLineEditItem(ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void hideButtons();
    void showButtons();
    QString getText();

    void setFocusOnSearchBar();
    void setSelected(bool selected);

    virtual void updateScaling();

signals:
    void searchModeExited();
    void textChanged(QString text);
    void enterPressed();
    void focusIn();

protected:
    void focusInEvent(QFocusEvent *event);

private slots:
    void onCancelClicked();
    void onClearTextClicked();
    void onLineEditKeyPress(QKeyEvent *keyEvent);
    void onLanguageChanged();
    void onLineEditTextChanged(QString text);
    void onSearchIconOpacityChanged(const QVariant & value);

private:
    double searchIconOpacity_;
    QGraphicsProxyWidget *proxyWidget_;
    CustomMenuLineEdit *lineEdit_;

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
