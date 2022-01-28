#ifndef OPENURLITEM_H
#define OPENURLITEM_H

#include <QLineEdit>

#include "baseitem.h"
#include "commongraphics/iconbutton.h"

namespace PreferencesWindow {

class OpenUrlItem : public BaseItem
{
    Q_OBJECT

public:
    explicit OpenUrlItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

public slots:
    void setText(const QString &text);
    void setUrl(const QString &url);
    void setUrl(std::function<QString()> set_url);

private slots:
    void onOpenUrl();

private:
    IconButton *button_;
    QString text_;
    QString url_;
    std::function<QString()> set_url_func_;
};

} // namespace PreferencesWindow

#endif // OPENURLITEM_H
