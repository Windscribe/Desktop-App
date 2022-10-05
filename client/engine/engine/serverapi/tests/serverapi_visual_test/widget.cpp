#include "widget.h"
#include "ui_widget.h"

#include "engine/failover/failover.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    authHash_ = "142257:3:1663921160:458801c564bc4ceac71b5bfc10177f2db7c7a129e7:397f679fadd36ba21cab7e82a5ab448bd843bb4c3f";

    connectStateController_ = new ConnectStateController_moc(this);
    connect(ui->rbConnected, &QRadioButton::clicked, [this]() {
        connectStateController_->setState(CONNECT_STATE_CONNECTED);
    });
    connect(ui->rbConnecting, &QRadioButton::clicked, [this]() {
        connectStateController_->setState(CONNECT_STATE_CONNECTING);
    });
    connect(ui->rbDisconnecting, &QRadioButton::clicked, [this]() {
        connectStateController_->setState(CONNECT_STATE_DISCONNECTING);
    });
    connect(ui->rbDisconnected, &QRadioButton::clicked, [this]() {
        connectStateController_->setState(CONNECT_STATE_DISCONNECTED);
    });

    networkDetectionManager_ = new NetworkDetectionManager_moc(this);
    connect(ui->cbIsOnline, &QCheckBox::stateChanged, [this]() {
        networkDetectionManager_->setOnline(ui->cbIsOnline->isChecked());
    });

    accessManager_ = new NetworkAccessManager(this);

    serverAPI_ = new server_api::ServerAPI(this, connectStateController_, accessManager_, networkDetectionManager_,
                                           new failover::Failover(nullptr,accessManager_, connectStateController_, "disconnected"),
                                           new failover::Failover(nullptr,accessManager_, connectStateController_, "connected"));

    connect(ui->btnFetchResources, &QPushButton::clicked, this, &Widget::onFetchResourcesClick);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onFetchResourcesClick()
{
    {
        server_api::BaseRequest *request = serverAPI_->session(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverLocations("", "df", false, PROTOCOL::OPENVPN_TCP, QStringList());
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverCredentials(authHash_, PROTOCOL::OPENVPN_TCP);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->serverConfigs(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->portMap(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->notifications(authHash_);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->pingTest(5000, true);
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
    {
        server_api::BaseRequest *request = serverAPI_->debugLog("aaa_test", "true");
        connect(request, &server_api::BaseRequest::finished, [request]() {
            request->deleteLater();
        });
    }
}



