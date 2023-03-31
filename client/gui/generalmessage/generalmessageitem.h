#pragma once

#include <QSet>

#include "igeneralmessagewindow.h"
#include "commongraphics/basepage.h"
#include "commongraphics/imageitem.h"
#include "commongraphics/listbutton.h"

namespace GeneralMessageWindow {

class GeneralMessageItem: public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit GeneralMessageItem(ScalableGraphicsObject *parent, int width, IGeneralMessageWindow::Style style);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setIcon(const QString &icon);
    void setTitle(const QString &title);
    void setDescription(const QString &desc);

    void setAcceptButtonStyle(IGeneralMessageWindow::Style style);
    void setAcceptText(const QString &text);
    void setRejectText(const QString &text);

    void setTitleSize(int size);

signals:
    void acceptClick();
    void rejectClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private slots:
    void onHoverAccept();
    void onHoverLeaveAccept();
    void onHoverReject();
    void onHoverLeaveReject();

private:
    IGeneralMessageWindow::Style style_;
    IGeneralMessageWindow::Shape shape_;

    bool isSpinnerMode_;

    QScopedPointer<ImageItem> icon_;
    QString title_;
    QString desc_;

    int titleSize_;
    int titleHeight_;
    int descHeight_;

    CommonGraphics::ListButton *acceptButton_;
    CommonGraphics::ListButton *rejectButton_;

    static constexpr int kSpacerHeight = 16;
    static constexpr int kIndent = 36;

    enum Selection { NONE, ACCEPT, REJECT };
    Selection selection_;

    void changeSelection(Selection selection);
    void updatePositions();
};

} // namespace GeneralMessageWindow
