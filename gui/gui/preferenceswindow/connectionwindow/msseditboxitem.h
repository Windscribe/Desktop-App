#ifndef MSSEDITBOXITEM_H
#define MSSEDITBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegExpValidator>
#include "commongraphics/iconbutton.h"
#include "commongraphics/custommenulineedit.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class MssEditBoxItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit MssEditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt, bool isDrawFullBottomDivider, const QString &additionalButtonIcon);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(const QString &text);
    void setValidator(QRegExpValidator *validator);

    void updateScaling() override;
    void setEditButtonClickable(bool clickable);

    void setAdditionalButtonBusyState(bool on);

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

    void onBusySpinnerOpacityAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationFinished();
    void onAdditionalButtonOpacityAnimationValueChanged(const QVariant &value);
private:
    QString caption_;
    QString text_;
    QString editPlaceholderText_;

    IconButton *btnEdit_;
    IconButton *btnAdditional_;
    IconButton *btnConfirm_;
    IconButton *btnUndo_;
    bool isEditMode_;

    QGraphicsProxyWidget *proxyWidget_;
    CustomMenuLineEdit *lineEdit_;
    DividerLine *line_;

    bool additionalButtonBusy_;
    double busySpinnerOpacity_;
    int busySpinnerRotation_;
    int spinnerPosX_;
    int spinnerPosY_;
    QVariantAnimation busySpinnerOpacityAnimation_;
    QVariantAnimation busySpinnerRotationAnimation_;

    double additionalButtonOpacity_;
    QVariantAnimation additionalButtonOpacityAnimation_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // MSSEDITBOXITEM_H
