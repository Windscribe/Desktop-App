#ifndef NEWIPORHOSTNAMEITEM_H
#define NEWIPORHOSTNAMEITEM_H

#include <QGraphicsProxyWidget>
#include "../baseitem.h"
#include "CommonGraphics/iconbutton.h"
#include "CommonGraphics/custommenulineedit.h"

namespace PreferencesWindow {

class NewIpOrHostnameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit NewIpOrHostnameItem(ScalableGraphicsObject *parent);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setSelected(bool selected);
    virtual void updateScaling();

signals:
    void addNewIpOrHostnameClicked(QString ipOrHostname);
    void escape();

private slots:
    void onCancelClicked();
    void onLineEditKeyPress(QKeyEvent *keyEvent);
    void onLanguageChanged();
    void onLineEditTextChanged(QString text);
    void onAddIpOrHostnameClicked();

private:
    QGraphicsProxyWidget *proxyWidget_;
    CustomMenuLineEdit *lineEdit_;

    bool editing_;
    IconButton *cancelTextButton_;
    IconButton *addIpOrHostnameButton_;
    DividerLine *line_;

    void setEditMode(bool editMode);
    void submitEntryForRule();

    void updatePositions();
};

}

#endif // NEWIPORHOSTNAMEITEM_H
