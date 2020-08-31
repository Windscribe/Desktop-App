#ifndef UPDATEAPPITEM_H
#define UPDATEAPPITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "iupdateappitem.h"
#include "commongraphics/textbutton.h"

namespace UpdateApp {

class UpdateAppItem : public ScalableGraphicsObject, public IUpdateAppItem
{
    Q_OBJECT
    Q_INTERFACES(IUpdateAppItem)
public:
    explicit UpdateAppItem(QGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setVersionAvailable(const QString &versionNumber) override;
    void setProgress(int value) override; // from 0 to 100

    QPixmap getCurrentPixmapShape() override;
    void updateScaling() override;

signals:
    void updateClick();

private slots:
    void onBackgroundOpacityChanged(const QVariant &value);
    void onVersionOpacityChanged(const QVariant &value);
    void onProgressForegroundOpacityChanged(const QVariant &value);
    void onProgressBackgroundOpacityChanged(const QVariant &value);

    void onProgressBarPosChanged(const QVariant &value);

    void onUpdateClick();

    void onLanguageChanged();

private:
    void animateTransitionToProgress();
    void animateTransitionToVersion();

    CommonGraphics::TextButton *updateButton_;
    bool inProgress_;
    QString curVersionText_;

    static constexpr int WIDTH = 230;
    static constexpr int HEIGHT = 20;

    double curBackgroundOpacity_;
    double curVersionOpacity_;

    double curProgressBackgroundOpacity_;
    double curProgressForegroundOpacity_;

    QVariantAnimation backgroundOpacityAnimation_;
    QVariantAnimation versionOpacityAnimation_;
    QVariantAnimation progressForegroundOpacityAnimation_;
    QVariantAnimation progressBackgroundOpacityAnimation_;

    int curProgressBarPos_;  //  0.0 -> 1.0
    QVariantAnimation progressBarPosChangeAnimation_;

    static constexpr int VERSION_TEXT_HEIGHT = 14;

    void updatePositions();
};

} // namespace UpdateApp

#endif // UPDATEAPPITEM_H
