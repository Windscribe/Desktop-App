#ifndef ROBERTWINDOWITEM_H
#define ROBERTWINDOWITEM_H

#include <QSharedPointer>

#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "commongraphics/basepage.h"
#include "commongraphics/bubblebuttondark.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/robertfilter.h"
#include "loadingiconitem.h"
#include "robertitem.h"

namespace PreferencesWindow {

class RobertWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit RobertWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const;
    void updateScaling() override;

    void setLoggedIn(bool loggedIn);
    void clearFilters();
    void setFilters(const QVector<types::RobertFilter> &filters);
    void setError(bool isError);
    void setLoading(bool loading);

signals:
    void accountLoginClick();
    void manageRobertRulesClick();
    void setRobertFilter(const types::RobertFilter &filter);

private:
    static constexpr int kMessageOffsetY = 85;
    static constexpr int kLoadingIconOffsetY = 215;
    static constexpr int kLoadingIconSize = 40;

    void updatePositions();
    void updateVisibility();

    PreferenceGroup *desc_;
    QList<PreferenceGroup *> groups_;

    LoadingIconItem *loadingIcon_;

    QGraphicsTextItem *loginPrompt_;
    CommonGraphics::BubbleButtonDark *loginButton_;

    QGraphicsTextItem *errorMessage_;
    bool loggedIn_;
    bool isError_;
    bool loading_;
};

} // namespace PreferencesWindow

#endif // ROBERTWINDOWITEM_H