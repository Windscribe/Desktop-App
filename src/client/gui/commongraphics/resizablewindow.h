#pragma once

#include <QKeyEvent>

#include "../common/types/enums.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/resizebar.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/scrollarea.h"

class ResizableWindow : public ScalableGraphicsObject
{
    Q_OBJECT

public:
    explicit ResizableWindow(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);
    ~ResizableWindow();

    QRectF boundingRect() const override;

    void setMinimumHeight(int height);
    int minimumHeight();
    void setMaximumHeight(int height);
    int maximumHeight();
    void setXOffset(int x);
    void setHeight(int height, bool ignoreMinimum = false);
    void setScrollBarVisibility(bool on);
    void updateScaling() override;
    int scrollPos();
    void setScrollPos(int pos);
    void setScrollOffset(int offset);
    void setVanGoghOffset(int offset);

    void setBackButtonEnabled(bool b);
    void setResizeBarEnabled(bool b);

signals:
    void escape();
    void sizeChanged(ResizableWindow *window);
    void resizeFinished(ResizableWindow *window);

public slots:
    virtual void onWindowExpanded();
    virtual void onWindowCollapsed();

protected slots:
    virtual void onResizeStarted();
    virtual void onResizeChange(int y);
    virtual void onResizeFinished();
    virtual void onBackArrowButtonClicked();
    virtual void onAppSkinChanged(APP_SKIN s);
    void onLanguageChanged();

protected:
    static constexpr int kBottomAreaHeight = 16;
    static constexpr int kBottomResizeOriginX = 160;
    static constexpr int kBottomResizeOffsetY = 13;
    static constexpr int kDefaultXOffset = 16;
    static constexpr int kVanGoghOffset = 16;
    static constexpr int kThrottleMs = 20;

    Preferences *preferences_;

    int minHeight_;
    int maxHeight_;
    int curHeight_;
    double curScale_;
    int heightAtResizeStart_;
    int xOffset_;
    int vanGoghOffset_;
    qint64 lastResizeUpdateTime_;
    int pendingResizeY_;
    bool hasPendingResize_;

    QString backgroundBase_;
    QString backgroundHeader_;
    QString backgroundBorder_;
    QString backgroundBorderExtension_;
    QString backgroundBorderBottom_;

    IconButton *backArrowButton_;
    CommonGraphics::ScrollArea *scrollAreaItem_;
    CommonGraphics::ResizeBar *bottomResizeItem_;
    CommonGraphics::EscapeButton *escapeButton_;
    QColor footerColor_;

    virtual QRectF getBottomResizeArea();
    void keyPressEvent(QKeyEvent *event) override;
    virtual void updatePositions();
};