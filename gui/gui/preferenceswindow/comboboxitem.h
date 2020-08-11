#ifndef COMBOBOXITEM_H
#define COMBOBOXITEM_H

#include <QGraphicsProxyWidget>
#include "baseitem.h"
#include "dividerline.h"
#include "commonwidgets/combomenuwidget.h"
#include "commongraphics/texticonbutton.h"
#include "graphicresources/fontdescr.h"

namespace PreferencesWindow {

struct ComboBoxItemDescr
{
    ComboBoxItemDescr() : isInitialized_(false) {}

    ComboBoxItemDescr(const QString &caption, const QVariant &userValue) :
        caption_(caption), userValue_(userValue), isInitialized_(true) {}

    QString caption() const { Q_ASSERT(isInitialized_); return caption_; }
    QVariant userValue() const { Q_ASSERT(isInitialized_); return userValue_; }

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


class ComboBoxItem : public BaseItem
{
    Q_OBJECT
public:
    explicit ComboBoxItem(ScalableGraphicsObject *parent, const QString &caption, const QString &tooltip, int height, QColor fillColor, int captionOffsX, bool bShowDividerLine, QString id = "");
    ~ComboBoxItem() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    bool hasItems();
    void addItem(const QString &caption, const QVariant &userValue);
    void removeItem(const QVariant &value);
    void setCurrentItem(QVariant value);
    QVariant currentItem() const;
    QString caption() const;

    void setLabelCaption(QString text);
    QString labelCaption() const;
    void setCaptionFont(const FontDescr &fontDescr);

    void clear();

    void hideMenu();
    QPointF getButtonScenePos() const;

    void setColorScheme(bool darkMode);

    void setClickable(bool clickable) override;

    void updateScaling() override;

signals:
    void currentItemChanged(QVariant value);
    void menuClosed();
    void buttonHoverEnter();
    void buttonHoverLeave();

private slots:
    void onMenuOpened();
    void onMenuItemSelected(QString text, QVariant data);
    void onMenuHidden();
    void onMenuSizeChanged(int width, int height);

    void onButtonWidthChanged(int newWidth);

private:
    QString strCaption_;
    QString strTooltip_;
    QColor fillColor_;
    int captionOffsX_;
    int initialHeight_;

    FontDescr captionFont_;

    DividerLine *dividerLine_;
    CommonGraphics::TextIconButton *button_;

    QList<ComboBoxItemDescr> items_;
    ComboBoxItemDescr curItem_;
    CommonWidgets::ComboMenuWidget *menu_;

};

} // namespace PreferencesWindow

#endif // COMBOBOXITEM_H
