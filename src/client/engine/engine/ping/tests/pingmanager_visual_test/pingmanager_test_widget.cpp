#include "pingmanager_test_widget.h"
#include "ui_pingmanager_test_widget.h"

#include <QFileDialog>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    QCoreApplication::setOrganizationName("Windscribe");
    QCoreApplication::setApplicationName("PingManagerTest");

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

    connect(ui->btnSetNetworkName, &QPushButton::clicked, [this]() {
        networkDetectionManager_->setNetworkName(ui->edNetworkName->text());
    });

    connect(ui->btnUpdateIps, &QPushButton::clicked, [this]() {
        updateIps();
    });

    accessManager_ = new NetworkAccessManager(this);
    pingHosts_ = new PingMultipleHosts(this, connectStateController_, accessManager_);
    pingManager_ = new PingManager(this, connectStateController_, networkDetectionManager_, pingHosts_, "pingData", "pingmanager.log");
    connect(pingManager_, &PingManager::pingInfoChanged, [this](const QString &ip, int timems) {
        qDebug() << "Finished ip: " << ip << " -> " << timems << "ms";
        if (pingManager_->isAllNodesHaveCurIteration()) {
            qDebug() << "All nodes have the same iteration";
        }
    });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::updateIps()
{
    QFile file("test_ips.txt");
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
    }

    QVector<PingIpInfo> ips;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("//"))
            continue;
        QStringList pars = line.split(";");
        Q_ASSERT(pars.size() == 2);
        ips << PingIpInfo { pars[0].trimmed(), pars[1].trimmed(), "City", "Nick", PingType::kCurl };
    }

    qDebug() << "IPs updated:" << ips.count();
    pingManager_->updateIps(ips);
}





