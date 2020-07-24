#ifndef EDITBOXITEM_H
#define EDITBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegExpValidator>
#include "commongraphics/iconbutton.h"
#include "commongraphics/custommenulineedit.h"
#include "dividerline.h"

namespace PreferencesWindow {

class EditBoxItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit EditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt, bool isDrawFullBottomDivider);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setText(const QString &text);
    void setValidator(QRegExpValidator *validator);

    virtual void updateScaling();
    void setEditButtonClickable(bool clickable);
    void setMasked(bool masked);

signals:
    void textChanged(const QString &text);
    void additionalButtonClicked();
    void additionalButtonHoverEnter();
    void additionalButtonHoverLeave();

private slots:
    void onEditClick();
    void onConfirmClick();
    void onUndoClick();

    void onLineEditKeyPress(QKeyEvent * event);

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
    CustomMenuLineEdit *lineEdit_;
    DividerLine *line_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // EDITBOXITEM_H
