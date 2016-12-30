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

#include "stream.h"

#include <libedah/utils.h>
#include <libedah/logger.h>

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

Stream::Stream(QObject *parent) :
    QObject(parent), streamActive(false), sc_shared(nullptr), sc_channels(1)
{
    QString localeStr = settings->value("lang", "").toString();
    if(localeStr.isEmpty())
    {
        localeStr = QLocale::system().name().left(2);
    }
    if(localeStr != "en")
    {
        if(!translator.load(QLocale(localeStr), "lang", ".", ":/stream-lang"))
        {
            LOG(QString("Couldn't load translation for \"%1\"").arg(localeStr));
        }
    }

    qApp->installTranslator(&translator);

        bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::start, this, &Stream::start);
    connect(bPanel, &BigPanel::stop, this, &Stream::stop);
    bPanel->retranslate();

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);


    this->settingsChanged();

    connect(this, &Stream::stateChanged, bPanel, &BigPanel::streamStateChanged);
    connect(this, &Stream::stateChanged, sPanel, &SmallPanel::streamStateChanged);

    connect(this, &Stream::positionChanged, bPanel, &BigPanel::streamPositionChanged);
    connect(this, &Stream::positionChanged, sPanel, &SmallPanel::streamPositionChanged);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Stream::refreshState);
    timer.start();

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(115, 50, 150), qRgb(255, 255, 0), qRgb(255, 0, 0));
}

Stream::~Stream()
{
    delete bPanel;
    delete sPanel;
    delete settingsTab;
}

QWidget *Stream::bigPanel()
{
    sPanel->removePeakMeter(peakMeter);
    bPanel->addPeakMeter(peakMeter);

    return bPanel;
}

QWidget *Stream::smallPanel()
{
    bPanel->removePeakMeter(peakMeter);
    sPanel->addPeakMeter(peakMeter);

    return sPanel;
}

bool Stream::hasPanel() const
{
    return true;
}

QWidget *Stream::getSettingsTab()
{
    return settingsTab;
}

QString Stream::getPluginName() const
{
    return tr("Stream");
}

QString Stream::getPluginId() const
{
    return "stream";
}

void Stream::loadSettings()
{
    settingsTab->loadSettings();
}

void Stream::writeSettings()
{
    settingsTab->writeSettings();
}

void Stream::settingsChanged()
{
    QLocale locale = QLocale(settings->value("lang", "").toString());
    translator.load(locale, "lang", ".", ":/stream-lang");

    settings->beginGroup(this->getPluginId());
    sc_channels = settings->value("sc_channels", 1).toInt();
    settings->endGroup();
}

void Stream::setPanelOpacity(int opacity)
{
    Q_UNUSED(opacity)
}

bool Stream::isActive()
{
    return streamActive;
}

void Stream::refreshState()
{
    if(sc_shared)
    {
        peakMeter->setPeak(sc_shared->levels[0], sc_shared->levels[1]);
        peakMeter->setChannels(sc_channels);
    }

    if(streamActive)
        emit positionChanged(QTime(0,0).addSecs(startTime.secsTo(QDateTime::currentDateTime())));
}

void Stream::createSharedMemory(QString name, Shared *&shared)
{
    HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*shared), name.toUtf8().data());

    if(handle == NULL)
    {
        LOG("CreateFileMapping: " + QString::number(GetLastError()));
    }

    shared = (Shared*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*shared));

    if(shared == NULL)
    {
        LOG("CreateFileMapping: " + QString::number(GetLastError()));
    }
}

void Stream::sc_start(int version, const QString &url, int port, int streamid, const QString &username, const QString &password, int channels, int bitrate, int samplerate, const QString &recDev)
{
    sc_channels = channels;

    QString urlArg = url + ":" + QString::number(port);
    if(version > 1)
        urlArg += "," + QString::number(streamid);

    QString passArg;
    if(!username.isEmpty())
        passArg = username + ":";
    passArg += password;

    QStringList args;
    args << "--version=" + QString::number(version)
         << "--url=" + urlArg
         << "--password=" + passArg
         << "--channels=" + QString::number(channels)
         << "--bitrate=" + QString::number(bitrate)
         << "--samplerate=" + QString::number(samplerate)
         << "--recDev=" + recDev;

    connect(&sc_process, &QProcess::readyReadStandardError, this, [this]() {
        QString output = QString::fromUtf8(sc_process.readAllStandardError());
        output.replace("\r\n", "\n");
        QStringList msg = output.split('\n');
        for(int i=0; i<msg.size(); i++)
        {
            if(msg[i].size() > 0)
            {
                if(msg[i] == "Start") msg[i] = tr("Starting");
                if(msg[i] == "OK") msg[i] = tr("OK");
                if(msg[i] == "End") msg[i] = tr("Ended");

                bPanel->addStatus("[SHOUTcast] " + msg[i]);
            }
        }
    });

    this->createSharedMemory("sc_shared", sc_shared);
    sc_shared->end = false;

    sc_process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    sc_process.start(utils->getDataDir() + "/plugins/stream/sc.exe", args);
}

void Stream::start()
{
    settings->beginGroup(this->getPluginId());
    QString recDev = settings->value("device", "").toString();

    bool sc_enabled = settings->value("shoutcast", false).toBool();

    if(sc_enabled)
    {
        this->sc_start(settings->value("sc_version", 1).toInt()+1,
                       settings->value("sc_url").toString(),
                       settings->value("sc_port").toString().toInt(),
                       settings->value("sc_streamid").toString().toInt(),
                       settings->value("sc_username").toString(),
                       QByteArray::fromBase64(settings->value("sc_password").toByteArray()),
                       settings->value("sc_channels", 1).toInt(),
                       settings->value("sc_bitrate", 64).toInt(),
                       settings->value("sc_sampleRate", 44100).toInt(),
                       recDev
                       );
    }

    settings->endGroup();

    streamActive = true;

    startTime = QDateTime::currentDateTime();

    emit stateChanged();
}

void Stream::stop()
{
    sc_shared->end = true;

    streamActive = false;

    emit stateChanged();
}
