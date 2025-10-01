#pragma once

#include <QGraphicsProxyWidget>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class NewAddressItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit NewAddressItem(ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setSelected(bool selected) override;
    void updateScaling() override;
    bool lineEditHasFocus();

    void setClickable(bool clickable) override;

signals:
    void addNewAddressClicked(QString address);
    void keyPressed();
    void escape();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCancelClicked();
    void onLanguageChanged();
    void onLineEditTextChanged(QString text);
    void onAddAddressClicked();

private:
    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

    bool editing_;
    IconButton *cancelTextButton_;
    IconButton *addAddressButton_;

    void setEditMode(bool editMode);
    void submitEntryForRule();

    void updatePositions();
};

}
