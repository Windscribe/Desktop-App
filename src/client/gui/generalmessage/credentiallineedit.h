#pragma once

#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include "commongraphics/baseitem.h"
#include "commonwidgets/custommenulineedit.h"

namespace GeneralMessageWindow {

class CredentialLineEdit : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit CredentialLineEdit(ScalableGraphicsObject *parent, const QString &buttonText, bool isPassword = false);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    QString text() const;
    void setLineEditText(const QString &text);
    void clear();

signals:
    void editingFinished();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    CommonWidgets::CustomMenuLineEdit *lineEdit_;
    QGraphicsProxyWidget *proxy_;

    QString text_;

    void updateStyleSheet();
};

} // namespace GeneralMessageWindow