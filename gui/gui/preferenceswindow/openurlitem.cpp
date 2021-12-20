#include <QDesktopServices>
#include <QUrl>

#include "openurlitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

OpenUrlItem::OpenUrlItem(ScalableGraphicsObject *parent)
    : BaseItem(parent, 50),
      set_url_func_(nullptr)
{
    button_ = new IconButton(16, 16, "preferences/EXTERNAL_LINK_ICON", "", this);
    button_->setPos(boundingRect().width() - button_->boundingRect().width() - 16, (boundingRect().height() - button_->boundingRect().height()) / 2);
    connect(button_, &IconButton::clicked, this, &OpenUrlItem::onOpenUrl);
    connect(button_, &IconButton::clicked, this, &OpenUrlItem::clicked);
}

void OpenUrlItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 1*G_SCALE, 0, -2*G_SCALE), Qt::AlignVCenter, text_);
}

void OpenUrlItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    button_->setPos(boundingRect().width() - button_->boundingRect().width() - 16*G_SCALE, (boundingRect().height() - button_->boundingRect().height()) / 2);
}

void OpenUrlItem::setText(const QString &text)
{
    text_ = text;
}

void OpenUrlItem::setUrl(const QString &url)
{
    url_ = url;
    set_url_func_ = nullptr;
}

void OpenUrlItem::setUrl(std::function<QString()> set_url)
{
    set_url_func_ = set_url;
    url_.clear();
}

void OpenUrlItem::onOpenUrl()
{
    QUrl url;
    if (url_.isEmpty() && set_url_func_ != nullptr) {
        url = QUrl(set_url_func_());
    }
    else {
        url = QUrl(url_);
    }

    // this call does nothing when url is empty string, as is the case for edit account details
    QDesktopServices::openUrl(url);
}

} // namespace PreferencesWindow
