#pragma once

#include <QGraphicsProxyWidget>
#include <QPair>
#include <QVariant>

#include "commonwidgets/combomenuwidget.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/texticonbutton.h"
#include "graphicresources/fontdescr.h"
#include "graphicresources/independentpixmap.h"
#include "utils/ws_assert.h"

namespace PreferencesWindow {

struct ComboBoxItemDescr
{
    ComboBoxItemDescr() : isInitialized_(false) {}

    ComboBoxItemDescr(const QString &caption, const QVariant &userValue) :
        caption_(caption), userValue_(userValue), isInitialized_(true) {}

    QString caption() const { WS_ASSERT(isInitialized_); return caption_; }
    void setCaption(const QString &caption) { caption_ = caption; }
    QVariant userValue() const { WS_ASSERT(isInitialized_); return userValue_; }

    bool isInitialized() const { return isInitialized_; }
    void clear() { isInitialized_ = false; }

    bool operator ==(const ComboBoxItemDescr& d)
    {
        return caption_ == d.caption_ && userValue_ == d.userValue_ && isInitialized_ == d.isInitialized_;
    }

private:
    QString caption_;
    QVariant userValue_;
    bool isInitialized_;
};


class ComboBoxItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ComboBoxItem(ScalableGraphicsObject *parent, const QString &caption = "", const QString &tooltip = "");
    ~ComboBoxItem() override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    bool hasItems();
    void addItem(const QString &caption, const QVariant &userValue);
    virtual void setItems(const QList<QPair<QString, QVariant>> &list, QVariant curValue = QVariant::fromValue(nullptr));
    void removeItem(const QVariant &value);
    virtual void setCurrentItem(QVariant value);
    QVariant currentItem() const;
    QString caption() const;

    void setLabelCaption(QString text);
    QString labelCaption() const;
    void setCaptionFont(const FontDescr &fontDescr);

    void setCaptionY(int y, bool animate = true);

    void clear();

    bool hasItemWithFocus() override;
    void hideMenu();
    void setMaxMenuItemsShowing(int maxItemsShowing);
    QPointF getButtonScenePos() const;

    void setColorScheme(bool darkMode);

    void setClickable(bool clickable) override;

    void updateScaling() override;

    void setIcon(QSharedPointer<IndependentPixmap> icon);
    void setButtonFont(const FontDescr &fontDescr);
    void setButtonIcon(const QString &icon);
    void setUseMinimumSize(bool useMinimumSize);

    void setDescription(const QString &description, const QString &url = "");
    void setInProgress(bool inProgress);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    int buttonWidth() const;

private:
    void recalculateCaptionY();

signals:
    void currentItemChanged(QVariant value);
    void buttonHoverEnter();
    void buttonHoverLeave();

private slots:
    void onMenuOpened();
    void onMenuItemSelected(QString text, QVariant data);
    void onMenuHidden();
    void onMenuSizeChanged(int width, int height);
    void onInfoIconClicked();
    void onCaptionPosUpdated(const QVariant &value);
    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();

private:
    QString strCaption_;
    QString strTooltip_;
    QColor fillColor_;

    FontDescr captionFont_;

    CommonGraphics::TextIconButton *button_;

    QList<ComboBoxItemDescr> items_;
    ComboBoxItemDescr curItem_;
    CommonWidgets::ComboMenuWidget *menu_;

    QSharedPointer<IndependentPixmap> icon_;
    bool isCaptionElided_ = false;
    QRectF captionRect_;

    QString desc_;
    QString descUrl_;
    IconButton *infoIcon_;
    int descHeight_ = 0;
    int descRightMargin_ = 0;

    int captionY_ = -1;
    int curCaptionY_ = -1;
    QVariantAnimation captionPosAnimation_;
    bool useMinimumSize_ = false;

    bool inProgress_;
    int spinnerRotation_;
    QVariantAnimation spinnerAnimation_;

    void updatePositions();
};

} // namespace PreferencesWindow
