#ifndef PACKETSIZEEDITBOXITEM_H
#define PACKETSIZEEDITBOXITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegularExpressionValidator>
#include <QTimer>
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"
#include "../dividerline.h"

namespace PreferencesWindow {

class PacketSizeEditBoxItem : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit PacketSizeEditBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &editPrompt, bool isDrawFullBottomDivider, const QString &additionalButtonIcon);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(const QString &text);
    void setValidator(QRegularExpressionValidator *validator);

    void updateScaling() override;
    void setEditButtonClickable(bool clickable);

    void setAdditionalButtonBusyState(bool on);
    void setAdditionalButtonSelectedState(bool selected);

    bool lineEditHasFocus();

signals:
    void textChanged(const QString &text);
    void additionalButtonClicked();
    void additionalButtonHoverEnter();
    void additionalButtonHoverLeave();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onEditClick();
    void onConfirmClick();
    void onUndoClick();

    void onLanguageChanged();

    void onBusySpinnerOpacityAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationFinished();
    void onBusySpinnerStartSpinning();
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
    CommonWidgets::CustomMenuLineEdit *lineEdit_;
    DividerLine *line_;

    bool additionalButtonBusy_;
    double busySpinnerOpacity_;
    int busySpinnerRotation_;
    int spinnerPosX_;
    int spinnerPosY_;
    QVariantAnimation busySpinnerOpacityAnimation_;
    QVariantAnimation busySpinnerRotationAnimation_;
    QTimer busySpinnerTimer_;

    double additionalButtonOpacity_;
    QVariantAnimation additionalButtonOpacityAnimation_;

    void updatePositions();

};

} // namespace PreferencesWindow


#endif // PACKETSIZEEDITBOXITEM_H
