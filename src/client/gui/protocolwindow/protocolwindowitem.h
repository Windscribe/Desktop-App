#pragma once

#include "backend/backend.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/resizablewindow.h"
#include "commongraphics/basepage.h"
#include "connectwindow/connectwindowitem.h"
#include "protocolwindow/protocolpromptitem.h"

namespace ProtocolWindow {

class ProtocolWindowItem : public ResizableWindow
{
    Q_OBJECT
public:
    explicit ProtocolWindowItem(QGraphicsObject *parent,
                                ConnectWindow::ConnectWindowItem *connectWindow,
                                Preferences *preferences,
                                PreferencesHelper *preferencesHelper);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setProtocolStatus(const types::ProtocolStatus &status);
    void setProtocolStatus(const QVector<types::ProtocolStatus> &status);
    void setMode(ProtocolWindowMode mode);
    void resetProtocolStatus();
    bool hasMoreAttempts();

private slots:
    void onPromptHeightChanged(int height);
    void onAppSkinChanged(APP_SKIN s) override;
    void onEscape();

signals:
    void protocolClicked(types::Protocol protocol, uint port);
    void stopConnection();

private:
    ProtocolPromptItem *protocolPromptItem_;
    int promptHeight_;
    bool allProtocolsFailed_;
};

}
