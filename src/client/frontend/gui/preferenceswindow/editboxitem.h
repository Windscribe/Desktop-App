#pragma once

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegularExpressionValidator>
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"
#include "ieditableitem.h"

namespace PreferencesWindow {

class EditBoxItem : public CommonGraphics::BaseItem, public IEditableItem
{
    Q_OBJECT

public:
    explicit EditBoxItem(ScalableGraphicsObject *parent, const QString &title = "", const QString &editPrompt = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCaption(const QString &caption);
    void setText(const QString &text);
    QString text() const { return text_; }
    void setPrompt(const QString &prompt);
    void setValidator(QRegularExpressionValidator *validator);
    void setMinimumLength(int length);

    void updateScaling() override;
    void setEditButtonClickable(bool clickable);
    void setMasked(bool masked);

    bool lineEditHasFocus();
    bool isInEditMode() const override;
    void save() override;
    void discard() override;

signals:
    void textChanged(const QString &text);
    void editClicked();
    void cancelled();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEditClick();
    void onConfirmClick();
    void onUndoClick();
    void onTextChanged(const QString &text);

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

    int minLength_;

    void updatePositions();

};

} // namespace PreferencesWindow
