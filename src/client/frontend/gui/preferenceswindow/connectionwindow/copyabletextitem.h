#pragma once

#include <QGraphicsObject>
#include <QTimer>
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "tooltips/tooltiptypes.h"

namespace PreferencesWindow {

class CopyableTextItem : public CommonGraphics::BaseItem
{
    Q_OBJECT

public:
    explicit CopyableTextItem(ScalableGraphicsObject *parent, const QString &extraButtonIcon = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setText(const QString &text);
    void setLabel(const QString &label);
    void cancelToolTip();

    void updateScaling() override;

signals:
    void textChanged(const QString &text);
    void extraButtonPressed();

private slots:
    void onCopyClick();

private:
    void updatePositions();

    QString label_;
    QString text_;
    IconButton *copyButton_;
    IconButton *extraButton_ = nullptr;
};

} // namespace PreferencesWindow
