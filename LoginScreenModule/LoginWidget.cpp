/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   LoginWidget.cpp
 *  @brief  Simple full screen login widget for OpenSim and realXend authentication methods.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"
#include "LoginWidget.h"
#include "MemoryLeakCheck.h"

#include <QPixmap>

LoginWidget::LoginWidget(Foundation::Framework *framework) :
    framework_(framework),
    progress_timer_(new QTimer(this))
{
    /*
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(this);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    */

    setupUi(this);
    move(0,0);
    setWindowState(Qt::WindowFullScreen);

    progressBar->hide();

    connect(pushButton_Connect, SIGNAL(clicked()), SLOT(ParseInputAndConnect()));
    connect(pushButton_Exit, SIGNAL(clicked()), SLOT(Exit()));

    // Specific for Tundra: Hide the start location to not to confuse users.
    label_StartLocation->hide();
    lineEdit_StartLocation->hide();

    connect(lineEdit_WorldAddress, SIGNAL(returnPressed()), SLOT(ParseInputAndConnect()));
    connect(lineEdit_StartLocation, SIGNAL(returnPressed()), SLOT(ParseInputAndConnect()));
    connect(lineEdit_Username, SIGNAL(returnPressed()), SLOT(ParseInputAndConnect()));
    connect(lineEdit_Password, SIGNAL(returnPressed()), SLOT(ParseInputAndConnect()));
    connect(progress_timer_, SIGNAL(timeout()), SLOT(UpdateProgressBar()));

    // Add the default realxtend logo
    label_ClientLogo->setPixmap(QPixmap("./data/ui/images/realxtend_logo.png"));

    ReadConfig();
}

QMap<QString, QString> LoginWidget::GetLoginInfo() const
{
    QMap<QString, QString> info;
    info["account"] = lineEdit_Username->text().trimmed();
    info["password"] = lineEdit_Password->text().trimmed();
    info["loginurl"] = lineEdit_WorldAddress->text().trimmed();
    info["startlocation"] = lineEdit_StartLocation->text().trimmed();

    if (lineEdit_Username->text().contains('@'))
        info["avatartype"] = "realxtend";
    else if (lineEdit_Username->text().trimmed().contains(' '))
        info["avatartype"] = "opensim";
    else
        // For now, use a single-part username to distinguish Tundra login
        info["avatartype"] = "tundra";

    info["protocol"] = "tcp";
    if (radioButton_ProtocolUDP->isChecked())
        info["protocol"] = "udp";

    return info;
}

void LoginWidget::SetLoginInfo(QMap<QString, QString> info)
{
    lineEdit_Username->setText(info["account"]);
    //lineEdit_Password->setText(info["password"]);
    lineEdit_WorldAddress->setText(info["loginurl"]);
    lineEdit_StartLocation->setText(info["startlocation"]);
    if (info["protocol"].toLower() == "tcp")
        radioButton_ProtocolTCP->setChecked(true);
    else if (info["protocol"].toLower() == "udp")
        radioButton_ProtocolUDP->setChecked(true);
}

void LoginWidget::ReadConfig()
{
    QMap<QString, QString> info;
    QString config_file = QString::fromStdString(framework_->GetPlatform()->GetApplicationDataDirectory()) + "/configuration/TundraLogin.ini";
    QSettings login_config(config_file, QSettings::IniFormat, this);
    info["account"] = login_config.value("account", QString()).toString();
    //info["password"] = login_config.value("account", QString()).toString(); 
    info["loginurl"] = login_config.value("loginurl", QString()).toString();
    info["startlocation"] = login_config.value("startlocation", QString()).toString();
    info["avatartype"] = login_config.value("avatartype", QString()).toString();
    info["protocol"] = login_config.value("protocol", QString()).toString();
    SetLoginInfo(info);
}

void LoginWidget::WriteConfig()
{
    QMap<QString, QString> login_info = GetLoginInfo();
    QString config_file = QString::fromStdString(framework_->GetPlatform()->GetApplicationDataDirectory()) + "/configuration/TundraLogin.ini";
    QSettings login_config(config_file, QSettings::IniFormat, this);
    foreach(QString key, login_info.keys())
    {
        // lets not store password
        if (key == "password")
            continue;
        QString value = login_info[key];
        login_config.setValue(key, value);
    }
}

void LoginWidget::ParseInputAndConnect()
{
    if (lineEdit_Username->text().isEmpty())
    {
        SetStatus(tr("Username is missing!"));
        return;
    }

    if (lineEdit_WorldAddress->text().isEmpty())
    {
        SetStatus(tr("World address is missing!"));
        return;
    }

    QMap<QString, QString> map;
    map["WorldAddress"] = lineEdit_WorldAddress->text().trimmed();
    map["Username"] = lineEdit_Username->text().trimmed();
    map["Password"] = lineEdit_Password->text().trimmed();
    map["StartLocation"] = lineEdit_StartLocation->text().trimmed();

    map["Protocol"] = "tcp";
    if (radioButton_ProtocolUDP->isChecked())
        map["Protocol"] = "udp";

    if (lineEdit_Username->text().contains('@'))
    {
        if (map["Username"].count("@") == 1 && map["Username"].count(" ") == 0)
        {
            map["AvatarType"] = "RealXtend";
            QString username_input = map["Username"];
            int at_index = username_input.indexOf("@");
            QString rex_username = username_input.midRef(0, at_index).toString();
            QString rex_auth_address = username_input.midRef(at_index+1).toString();

            map["Username"] = rex_username;
            map["AuthenticationAddress"] = rex_auth_address;
            emit Connect(map);
        }
    }
    else
    {
        if (map["Username"].count(" ") == 1)
        {
            map["AvatarType"] = "OpenSim";
            emit Connect(map);
        }
        else
        {
            // For now, use a single-part username to distinguish Tundra login
            map["AvatarType"] = "Tundra";
            emit Connect(map);
        }
    }
}

void LoginWidget::SetStatus(const QString &message)
{
    statusLabel->setText(message);
}

void LoginWidget::Connected()
{
    SetStatus("Connected");
    WriteConfig();
}

void LoginWidget::StartProgressBar()
{
    SetStatus(QString(tr("Connecting to %1 with %2")).arg(lineEdit_WorldAddress->text(), lineEdit_Username->text()));

    progressBar->setValue(0);
    progressBar->show();
    progress_timer_->start(30);
}

void LoginWidget::UpdateProgressBar()
{
    int value = progressBar->value();
    if (value == 0)
        progress_direction_ = 1;
    else if (value == 100)
        progress_direction_ = -1;
    progressBar->setValue(value + progress_direction_);
}

void LoginWidget::StopProgressBar()
{
    progress_timer_->stop();
    progressBar->hide();
}

void LoginWidget::Exit()
{
    emit ExitClicked();
}