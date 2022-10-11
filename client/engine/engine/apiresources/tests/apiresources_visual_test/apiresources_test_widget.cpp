#include "apiresources_test_widget.h"
#include "ui_apiresources_test_widget.h"

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

    apiResourceManager_ = new api_resources::ApiResourcesManager(this, serverAPI_, connectStateController_, networkDetectionManager_);
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::readyForLogin, [this] {
        qDebug() << "ApiResourcesManager::readyForLogin";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::loginFailed, [this](LOGIN_RET loginRetCode, const QString &errorMessage) {
        qDebug() << "ApiResourcesManager::loginFailed, retCode =" << LOGIN_RET_toString(loginRetCode) << ";errorMessage =" << errorMessage;
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::sessionDeleted, [this]() {
        qDebug() << "ApiResourcesManager::sessionDeleted";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::sessionUpdated, [this]() {
        qDebug() << "ApiResourcesManager::sessionUpdated";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::locationsUpdated, [this]() {
        qDebug() << "ApiResourcesManager::locationsUpdated";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::staticIpsUpdated, [this]() {
        qDebug() << "ApiResourcesManager::staticIpsUpdated";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::notificationsUpdated, [this]() {
        qDebug() << "ApiResourcesManager::notificationsUpdated";
    });
    connect(apiResourceManager_, &api_resources::ApiResourcesManager::serverCredentialsFetched, [this]() {
        qDebug() << "ApiResourcesManager::serverCredentialsFetched";
    });

    checkUpdateManager_ = new api_resources::CheckUpdateManager(this, serverAPI_);
    connect(checkUpdateManager_, &api_resources::CheckUpdateManager::checkUpdateUpdated, [this] () {
        qDebug() << "CheckUpdateManager::checkUpdateUpdated";
    });

    myIpManager_ = new api_resources::MyIpManager(this, serverAPI_, networkDetectionManager_, connectStateController_);
    connect(myIpManager_, &api_resources::MyIpManager::myIpChanged, [this](const QString &ip, bool isFromDisconnectedState) {
        qDebug() << "MyIpManager::myIpChanged" << ip << "isFromDisconnectedState:" << isFromDisconnectedState;
    });

    connect(ui->btnLogin, &QPushButton::clicked, [this] {
        apiResourceManager_->login(ui->edUsername->text(), ui->edPassword->text(), "");
    });

    connect(ui->btnFetchWithAuthHash, &QPushButton::clicked, [this] {
        Q_ASSERT(apiResourceManager_->isAuthHashExists());
        apiResourceManager_->fetchAllWithAuthHash();
    });

    connect(ui->btnFetchSession, &QPushButton::clicked, [this] {
        apiResourceManager_->fetchSession();
    });

    connect(ui->btnCheckUpdate, &QPushButton::clicked, [this] {
        checkUpdateManager_->checkUpdate(UPDATE_CHANNEL_RELEASE);
    });
    connect(ui->btnMyIp, &QPushButton::clicked, [this] {
        myIpManager_->getIP(1);
    });
    connect(ui->btnRefetchServerCredentials, &QPushButton::clicked, [this] {
        apiResourceManager_->fetchServerCredentials();
    });

}

Widget::~Widget()
{
    delete ui;
}





