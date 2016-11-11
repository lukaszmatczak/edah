/*
    Edah
    Copyright (C) 2016  Lukasz Matczak

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settingstab.h"

#include <libedah/utils.h>

#include <QFormLayout>
#include <QPushButton>
#include <QApplication>
#include <QStyle>
#include <QFileDialog>
#include <QLabel>
#include <QScreen>
#include <QMessageBox>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QDebug>

QVector<AudioInfo> SettingsTab::ainfo;

SettingsTab::SettingsTab(IPlugin *parent) : QWidget(0), plugin(parent)
{
    QFormLayout *layout = new QFormLayout;
    this->setLayout(layout);

    this->setStyleSheet("#monitorsFrame {"
                        "   background-color: gray;"
                        "}"
                        "#monitorsFrame .QPushButton {"
                        "   border: 1px solid black;"
                        "   font-size: 14px;"
                        "   color: white;"
                        "}");

    selectedScreenLbl = new QLabel(this);
    layout->addRow(tr("Output video device: "), selectedScreenLbl);

    monitorsFrame = new QFrame(this);
    monitorsFrame->setObjectName("monitorsFrame");
    //monitorsFrame->setMinimumHeight(100);
    monitorsFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addRow("", monitorsFrame);

    connect(qApp, &QApplication::screenAdded, this, [this](QScreen*){ drawMonitors(); });
    connect(qApp, &QApplication::screenRemoved, this, [this](QScreen*){ drawMonitors(); });

    playDevBox = new QComboBox(this);
    playDevBox->setEditable(true);
    playDevBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addRow(tr("Audio device: "), playDevBox);

    QHBoxLayout *songsLayout = new QHBoxLayout;

    songsDir = new QLineEdit(this);
    songsLayout->addWidget(songsDir);

    QPushButton *songsDirBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DirIcon), "", this);
    songsDirBtn->setToolTip(tr("Select directory"));
    connect(songsDirBtn, &QPushButton::clicked, this, &SettingsTab::songsDirBtn_clicked);
    songsLayout->addWidget(songsDirBtn);

    layout->addRow(tr("Songs directory: "), songsLayout);

    downloadChk = new QGroupBox(tr("Automatically download videos from jw.org"), this);
    downloadChk->setCheckable(true);
    QFormLayout *downloadChkLayout = new QFormLayout;
    downloadChk->setLayout(downloadChkLayout);
    layout->addRow(downloadChk);

    QHBoxLayout *downloadLayout = new QHBoxLayout;

    downloadDir = new QLineEdit(downloadChk);
    downloadLayout->addWidget(downloadDir);

    QPushButton *downloadDirBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DirIcon), "", downloadChk);
    downloadDirBtn->setToolTip(tr("Select directory"));
    connect(downloadDirBtn, &QPushButton::clicked, this, &SettingsTab::downloadDirBtn_clicked);
    downloadLayout->addWidget(downloadDirBtn);

    downloadChkLayout->addRow(tr("Downloads directory: "), downloadLayout);

    downloadQuality = new QComboBox(downloadChk);
    downloadQuality->addItems({"240p", "360p", "480p", "720p"});
    downloadChkLayout->addRow(tr("Video quality: "), downloadQuality);

    downloadEmail = new QLineEdit(downloadChk);
    downloadChkLayout->addRow(tr("Email address: "), downloadEmail);

    QLabel *emailLbl = new QLabel(tr("Valid email address is required. You will receive report about downloaded files every Sunday."), downloadChk);
    emailLbl->setWordWrap(true);
    downloadChkLayout->addRow("", emailLbl);
}

void SettingsTab::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)

    monitorsFrame->setMinimumHeight(this->height()/4); // TODO

    this->drawMonitors();
}

void SettingsTab::setScreenLbl()
{
    foreach (QScreen *screen, QGuiApplication::screens())
    {
        if(screen->geometry() == selectedMonitor)
        {
            QString friendlyName = utils->getFriendlyName(screen->name());
            friendlyName += QString(" [%1x%2]").arg(screen->size().width()).arg(screen->size().height());
            selectedScreenLbl->setText(friendlyName);
        }
    }
}

void SettingsTab::drawMonitors()
{
    QList<QScreen*> screens = QGuiApplication::screens();

    double scale = qMin((monitorsFrame->height()-10.0)/screens[0]->virtualSize().height(),
            (monitorsFrame->width()-10.0)/screens[0]->virtualSize().width());

    while(QPushButton *w = monitorsFrame->findChild<QPushButton*>())
        delete w;

    int minX = screens[0]->virtualGeometry().x();
    int minY = screens[0]->virtualGeometry().y();

    foreach(QScreen *screen, screens)
    {
        QString name = QString("%1x%2").arg(screen->size().width()).arg(screen->size().height());
        QPushButton *monitorBtn = new QPushButton(monitorsFrame);

        if(screen->geometry() == QGuiApplication::primaryScreen()->geometry())
        {
            name += tr("\nMain");
            monitorBtn->setStyleSheet("background-color: rgb(76,76,76);");
        }
        else if(screen->geometry() == selectedMonitor)
        {
            monitorBtn->setStyleSheet("background-color: rgb(0,62,198);");
            monitorBtn->setCursor(QCursor(Qt::PointingHandCursor));
        }
        else
        {
            monitorBtn->setStyleSheet("background-color: rgb(36,36,76);");
            monitorBtn->setCursor(QCursor(Qt::PointingHandCursor));
        }

        monitorBtn->setText(name);

        QRect geom = screen->geometry();
        geom.translate(-minX, -minY);
        monitorBtn->setGeometry(QRect(geom.topLeft()*scale+QPoint(5,5), geom.size()*scale));

        connect(monitorBtn, &QPushButton::clicked, this, [this, screen](){
            if(screen->geometry() == QGuiApplication::primaryScreen()->geometry())
                QMessageBox::warning(this, "", tr("You couldn't choose main display!"));
            else
            {
                selectedMonitor = screen->geometry();
                drawMonitors();
            }
        });

        monitorBtn->show();
    }

    this->setScreenLbl();
}

void SettingsTab::songsDirBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, QString(), songsDir->text());
    if(!path.isEmpty())
    {
        songsDir->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTab::downloadDirBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, QString(), downloadDir->text());
    if(!path.isEmpty())
    {
        downloadDir->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTab::loadSettings()
{
    //AudioInfo
    playDevBox->clear();
    AudioInfo savedAinfo(plugin->getPluginId());

    int foundAt = -1;
    for(int i=0; i<ainfo.size(); i++)
    {
        if(ainfo[i] == savedAinfo) foundAt = i;
    }

    if(foundAt == -1 && !savedAinfo.id.isEmpty())
    {
        ainfo.push_front(savedAinfo);
        foundAt = 0;
    }

    foreach(AudioInfo info, ainfo)
    {
        playDevBox->addItem(info.toString(), QVariant::fromValue(info));
    }

    playDevBox->setCurrentIndex(foundAt);

    settings->beginGroup(plugin->getPluginId());
    selectedMonitor = settings->value("displayGeometry").toRect();
    songsDir->setText(settings->value("songsDir", "").toString());
    downloadChk->setChecked(settings->value("download", false).toBool());
    downloadDir->setText(settings->value("downloadDir", "").toString());
    downloadQuality->setCurrentText(settings->value("downloadQualoty", "720p").toString());
    downloadEmail->setText(settings->value("downloadEmail").toString());
    settings->endGroup();

    this->setScreenLbl();
    this->drawMonitors();
}

void SettingsTab::writeSettings()
{
    QRegExp rx("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", Qt::CaseInsensitive);
    if(!rx.exactMatch(downloadEmail->text()))
    {
        downloadChk->setChecked(false);
        QMessageBox msg(QMessageBox::Warning,
                        tr("Warning!"),
                        tr("You haven't entered valid email address! Auto-download will be disabled."),
                        QMessageBox::Ok,
                        this);
        msg.exec();
    }

    AudioInfo ainfo = playDevBox->itemData(playDevBox->currentIndex()).value<AudioInfo>();
    ainfo.save(plugin->getPluginId());

    settings->beginGroup(plugin->getPluginId());
    settings->setValue("displayGeometry", selectedMonitor);
    settings->setValue("songsDir", songsDir->text());
    settings->setValue("download", downloadChk->isChecked());
    settings->setValue("downloadDir", downloadDir->text());
    settings->setValue("downloadQuality", downloadQuality->currentText());
    settings->setValue("downloadEmail", downloadEmail->text());
    settings->endGroup();


    QNetworkAccessManager manager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/set_mail.php"));
    url.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager.post(url, QString("device=%1&user=%2&mail=%3&lang=%4")
                                       .arg(utils->getDeviceId())
                                       .arg(utils->getUserId())
                                       .arg(QString((downloadChk->isChecked() ? downloadEmail->text() : "").toUtf8().toBase64()))
                                       .arg(tr("en")).toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();
}
