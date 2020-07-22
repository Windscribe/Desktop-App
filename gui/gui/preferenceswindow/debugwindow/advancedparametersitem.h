#ifndef ADVANCEDPARAMETERSITEM_H
#define ADVANCEDPARAMETERSITEM_H

#include <QGraphicsProxyWidget>
#include <QPlainTextEdit>
#include "../baseitem.h"
#include "CommonGraphics/bubblebuttonbright.h"
#include "CommonGraphics/bubblebuttondark.h"
#include "CommonWidgets/customtexteditwidget.h"
#include "CommonWidgets/scrollareawidget.h"

namespace PreferencesWindow {

class AdvancedParametersItem : public BaseItem
{
    Q_OBJECT
public:
    explicit AdvancedParametersItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    void setAdvancedParameters(const QString &advParams);

    void updateWidgetPositions();
    void updateScaling();

signals:
    void advancedParametersChanged(const QString &advParms);

private slots:
    void onLanguageChanged();

    void onClearClicked();
    void onSaveClicked();

    void onTextEditTextChanged();

    void onHeightChanged(int newHeight);

    void onSaveTextColorChanged(const QVariant &value);
    void onSaveFillOpacityChanged(const QVariant &value);

private:
    QGraphicsProxyWidget *proxyWidget_;
    CustomTextEditWidget *textEdit_;

    ScrollAreaWidget *scrollArea_;

    CommonGraphics::BubbleButtonDark *clearButton_;
    CommonGraphics::BubbleButtonBright *saveButton_;

    const int TEXT_MARGIN_WIDTH = 32;
    const int TEXT_MARGIN_HEIGHT = 32;

    const int RECT_MARGIN_WIDTH = 16;
    const int RECT_MARGIN_HEIGHT = 16;

    QPropertyAnimation saveTextColorAnimation_;
    QVariantAnimation saveFillOpacityAnimation_;

    bool saved_;

};

} // namespace PreferencesWindow

#endif // ADVANCEDPARAMETERSITEM_H
