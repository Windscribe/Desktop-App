#pragma once

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QRegularExpressionValidator>
#include <QTimer>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commonwidgets/custommenulineedit.h"

namespace PreferencesWindow {

class PacketSizeEditBoxItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit PacketSizeEditBoxItem(ScalableGraphicsObject *parent, const QString &caption = "", const QString &editPrompt = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setCaption(const QString &caption);
    void setPrompt(const QString &prompt);
    void setText(const QString &text);
    void setValidator(QRegularExpressionValidator *validator);

    void updateScaling() override;
    void setEditButtonClickable(bool clickable);

    void setDetectButtonBusyState(bool on);
    void setDetectButtonSelectedState(bool selected);

    bool lineEditHasFocus();

signals:
    void textChanged(const QString &text);
    void detectButtonClicked();
    void detectButtonHoverEnter();
    void detectButtonHoverLeave();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEditClick();
    void onConfirmClick();
    void onUndoClick();

    void onBusySpinnerOpacityAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationChanged(const QVariant &value);
    void onBusySpinnerRotationAnimationFinished();
    void onBusySpinnerStartSpinning();
    void onAdditionalButtonOpacityAnimationValueChanged(const QVariant &value);
private:
    QString caption_;
    QString text_;
    QString editPlaceholderText_;

    IconButton *editButton_;
    IconButton *detectButton_;
    IconButton *confirmButton_;
    IconButton *undoButton_;
    bool isEditMode_;

    QGraphicsProxyWidget *proxyWidget_;
    CommonWidgets::CustomMenuLineEdit *lineEdit_;

    bool detectButtonBusy_;
    double busySpinnerOpacity_;
    int busySpinnerRotation_;
    int spinnerPosX_;
    int spinnerPosY_;
    QVariantAnimation busySpinnerOpacityAnimation_;
    QVariantAnimation busySpinnerRotationAnimation_;
    QTimer busySpinnerTimer_;

    double detectButtonOpacity_;
    QVariantAnimation detectButtonOpacityAnimation_;

    void updatePositions();
};

} // namespace PreferencesWindow
