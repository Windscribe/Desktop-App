#pragma once

#include "../backend/backend.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/resizablewindow.h"
#include "commongraphics/basepage.h"
#include "connectwindow/iconnectwindow.h"
#include "protocolwindow/iprotocolwindow.h"
#include "protocolwindow/protocolpromptitem.h"

namespace ProtocolWindow {

class ProtocolWindowItem : public IProtocolWindow
{
    Q_OBJECT
    Q_INTERFACES(IProtocolWindow)
public:
    explicit ProtocolWindowItem(QGraphicsObject *parent,
                                IConnectWindow *connectWindow,
                                Preferences *preferences,
                                PreferencesHelper *preferencesHelper);

    void setProtocolStatus(const types::ProtocolStatus &status) override;
    void setProtocolStatus(const QVector<types::ProtocolStatus> &status) override;
    void setMode(ProtocolWindowMode mode) override;
    void resetProtocolStatus() override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    bool hasMoreAttempts() override;

private slots:
    void onPromptHeightChanged(int height);
    void onAppSkinChanged(APP_SKIN s) override;
    void onEscape();

signals:
    void protocolClicked(types::Protocol protocol, uint port) override;
    void stopConnection() override;

private:
    ProtocolPromptItem *protocolPromptItem_;
    int promptHeight_;
    bool allProtocolsFailed_;
};

}
