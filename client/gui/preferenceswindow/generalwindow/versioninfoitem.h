#ifndef VERSIONINFOITEM_H
#define VERSIONINFOITEM_H

#include "../baseitem.h"
#include <QMenu>

namespace PreferencesWindow {

class VersionInfoItem : public BaseItem
{
    Q_OBJECT
public:
    explicit VersionInfoItem(ScalableGraphicsObject *parent, const QString &caption, const QString &value);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

private:
    QString strCaption_;
    QString strValue_;
};

} // namespace PreferencesWindow

#endif // VERSIONINFOITEM_H
