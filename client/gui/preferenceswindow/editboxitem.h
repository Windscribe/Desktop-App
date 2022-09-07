#ifndef EDITBOXITEM_H
#define EDITBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegularExpressionValidator>
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class EditBoxItem : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit EditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(const QString &text);
    void setValidator(QRegularExpressionValidator *validator);

    void updateScaling() override;
    void setEditButtonClickable(bool clickable);
    void setMasked(bool masked);

    bool lineEditHasFocus();

signals:
    void textChanged(const QString &text);
    void additionalButtonClicked();
    void additionalButtonHoverEnter();
    void additionalButtonHoverLeave();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEditClick();
    void onConfirmClick();
    void onUndoClick();

    void onLanguageChanged();

private:
    QString caption_;
    QString text_;
    QString editPlaceholderText_;

    IconButton *btnEdit_;
    IconButton *btnConfirm_;
    IconButton *btnUndo_;
    bool isEditMode_;
    QChar maskingChar_;

    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // EDITBOXITEM_H
