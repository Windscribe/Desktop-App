#pragma once

#include "commongraphics/baseitem.h"
#include "commongraphics/iconbutton.h"
#include "graphicresources/independentpixmap.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

class PreferenceGroup : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit PreferenceGroup(ScalableGraphicsObject *parent, const QString &desc = "", const QString &descUrl = "");
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void clearItems(bool skipFirst = false);
    void addItem(CommonGraphics::BaseItem *item, bool isWideDividerLine = false);

    void setDescription(const QString &desc, bool error = false);
    void setDescription(const QString &desc, const QString &descUrl);
    void showDescription();
    void hideDescription();
    void setDrawBackground(bool draw);
    void setDescriptionBorderWidth(int width);
    void clearError();

    int size();
    int indexOf(CommonGraphics::BaseItem *item);

    enum DISPLAY_FLAGS
    {
        FLAG_NONE = 0,
        FLAG_NO_ANIMATION = 1,
        FLAG_DELETE_AFTER = 2,
    };
    void showItems(int start, int end = -1, uint32_t flags = 0);
    void hideItems(int start, int end = -1, uint32_t flags = 0);

signals:
    void itemsChanged();

protected:
    const QList<CommonGraphics::BaseItem *> items() const;

private slots:
    void updatePositions();
    void onHidden();
    void onIconClicked();

private:
    int firstVisibleItem(int start = 0);
    bool isDividerLine(int index);

    QString desc_;
    QString errorDesc_;
    QString descUrl_;
    int height_;
    int descRightMargin_;
    int descHeight_;
    int borderWidth_;
    IconButton *icon_;
    QSharedPointer<IndependentPixmap> errorIcon_;
    bool error_;
    bool drawBackground_;
    bool descHidden_;

    QVariantAnimation descAnimation_;
    double descAnimationProgress_;

    QList<BaseItem *> itemsExternal_;
    QList<BaseItem *> items_;
    QList<BaseItem *> toDelete_;

};

} // namespace PreferencesWindow
