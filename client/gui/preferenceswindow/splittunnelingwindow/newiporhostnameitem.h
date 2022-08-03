#ifndef NEWIPORHOSTNAMEITEM_H
#define NEWIPORHOSTNAMEITEM_H

#include <QGraphicsProxyWidget>
#include "../baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class NewIpOrHostnameItem : public BaseItem
{
    Q_OBJECT
public:
    explicit NewIpOrHostnameItem(ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setSelected(bool selected) override;
    void updateScaling() override;
    bool lineEditHasFocus();

signals:
    void addNewIpOrHostnameClicked(QString ipOrHostname);
    void escape();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCancelClicked();
    void onLanguageChanged();
    void onLineEditTextChanged(QString text);
    void onAddIpOrHostnameClicked();

private:
    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

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
