#pragma once

#include <QPlainTextEdit>
#include "commonwidgets/custommenulineedit.h"
#include "commonwidgets/iscrollablewidget.h"

class CustomTextEditWidget : public QPlainTextEdit, public IScrollableWidget
{
    Q_OBJECT
    Q_INTERFACES(IScrollableWidget)

public:
    CustomTextEditWidget(QWidget *parent = nullptr);
    QWidget * getWidget() override { return this; }

    QSize sizeHint() const override;

    void setWidth(int width);
    void setViewportHeight(int height) override;

    void undoableClear();

    int stepSize() const override;
    void setStepSize(int stepSize) override;

    int lineHeight();
    void updateScaling();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

signals:
    void itemClicked(QString caption, QVariant value);

    void heightChanged(int newHeight) override;
    void shiftPos(int newPos) override;

    void recenterWidget() override;

private slots:
    void onMenuTriggered(QAction *action);

    void onUndoAvailableChanged(bool avail);
    void onCopyAvailableChanged(bool avail);
    void onRedoAvailableChanged(bool avail);

    void onTextChanged();
    void onCursorPosChanged();
    void onBlockCountChanged(int newblockCount);

private:
    int width_;
    int height_;
    int viewportHeight_;
    int stepSize_;

    bool pressed_;

    bool recalculatingHeight_ = false;

    CustomMenuWidget *menu_;

    void updateSelectAllAvailability();
    void disableActions();

    int firstVisibleLine();
    int lastVisibleLine();

    void shiftToCursor();

    void recalcHeight();

};
