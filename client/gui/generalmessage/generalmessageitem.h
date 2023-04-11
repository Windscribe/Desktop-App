#pragma once

#include <QCheckBox>
#include <QGraphicsProxyWidget>
#include <QSet>

#include "igeneralmessagewindow.h"
#include "commongraphics/basepage.h"
#include "commongraphics/checkbox.h"
#include "commongraphics/imageitem.h"
#include "commongraphics/listbutton.h"
#include "commongraphics/textbutton.h"
#include "languagecontroller.h"

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
    void setTertiaryText(const QString &text);

    void setTitleSize(int size);

    void setShowBottomPanel(bool on);
    void setLearnMoreUrl(const QString &url);
    bool isRememberChecked();

signals:
    void acceptClick();
    void rejectClick();
    void tertiaryClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool sceneEvent(QEvent *event) override;

private slots:
    void onHoverAccept();
    void onHoverReject();
    void onHoverTertiary();
    void onHoverLeave();
    void onLearnMoreClick();
    void onLanguageChanged();

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
    CommonGraphics::ListButton *tertiaryButton_;

    bool showBottomPanel_;
    Checkbox *checkbox_;
    QString learnMoreUrl_;
    CommonGraphics::TextButton *learnMoreLink_;

    const char *kRemember = QT_TR_NOOP("Ignore Warnings");
    const char *kLearnMore = QT_TR_NOOP("Learn More");

    static constexpr int kSpacerHeight = 16;
    static constexpr int kIndent = 36;

    enum Selection { NONE, ACCEPT, REJECT, TERTIARY };
    Selection selection_;

    void changeSelection(Selection selection);
    void updatePositions();
};

} // namespace GeneralMessageWindow
