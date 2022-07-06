#ifndef ADVANCEDPARAMETERSITEM_H
#define ADVANCEDPARAMETERSITEM_H

#include <QGraphicsProxyWidget>
#include <QPlainTextEdit>
#include "commongraphics/baseitem.h"
#include "commongraphics/bubblebuttonbright.h"
#include "commongraphics/bubblebuttondark.h"
#include "commonwidgets/customtexteditwidget.h"
#include "commonwidgets/scrollareawidget.h"

namespace PreferencesWindow {

class AdvancedParametersItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit AdvancedParametersItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setAdvancedParameters(const QString &advParams);

    void updateWidgetPositions();
    void updateScaling() override;

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

    // NOTE: ScrollAreaWidget's VerticalScrollbarWidget was updated to improve ComboBox scrollbar. When we return to custom widget for AdvancedParameters, verify scrollbar behaviour is okay.
    ScrollAreaWidget *scrollArea_;

    CommonGraphics::BubbleButtonDark *clearButton_;
    CommonGraphics::BubbleButtonBright *saveButton_;

    static constexpr int TEXT_MARGIN_WIDTH = 32;
    static constexpr int TEXT_MARGIN_HEIGHT = 32;

    static constexpr int RECT_MARGIN_WIDTH = 16;
    static constexpr int RECT_MARGIN_HEIGHT = 16;

    QPropertyAnimation saveTextColorAnimation_;
    QVariantAnimation saveFillOpacityAnimation_;

    bool saved_;

};

} // namespace PreferencesWindow

#endif // ADVANCEDPARAMETERSITEM_H
