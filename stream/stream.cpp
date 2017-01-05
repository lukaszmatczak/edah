/*
    Edah
    Copyright (C) 2016-2017  Lukasz Matczak

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
    QObject(parent), sc_shared(nullptr), sc_channels(1), voip_shared(nullptr), sc_status(STOPPED), voip_status(STOPPED)
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

    peakMeter = new PeakMeter;

    this->settingsChanged();

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::start, this, &Stream::start);
    connect(bPanel, &BigPanel::stop, this, &Stream::stop);
    bPanel->retranslate();

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);

    connect(this, &Stream::stateChanged, bPanel, &BigPanel::streamStateChanged);
    connect(this, &Stream::stateChanged, sPanel, &SmallPanel::streamStateChanged);

    connect(this, &Stream::positionChanged, bPanel, &BigPanel::streamPositionChanged);
    connect(this, &Stream::positionChanged, sPanel, &SmallPanel::streamPositionChanged);

    connect(&sc_process, &QProcess::readyReadStandardError, this, &Stream::sc_processRead);
    connect(&sc_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Stream::sc_processFinished);

    connect(&voip_process, &QProcess::readyReadStandardError, this, &Stream::voip_processRead);
    connect(&voip_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Stream::voip_processFinished);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Stream::refreshState);
    timer.start();
}

Stream::~Stream()
{
    this->stop();

    sc_process.waitForFinished(10000);
    voip_process.waitForFinished(10000);

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
    bool voipPlay = (settings->value("voip_playDev", "Mute").toString() != "Mute") && settings->value("voip", false).toBool();
    int scChannels = settings->value("shoutcast", false).toBool() ? sc_channels : 0;


    if(!settings->value("shoutcast", false).toBool())
    {
        sc_status = DISABLED;
    }

    if(!settings->value("voip", false).toBool())
    {
        voip_status = DISABLED;
    }

    settings->endGroup();

    if(scChannels == 0)
    {
        if(voipPlay)
        {
            peakLeft = VOIP_PLAY;
            peakRight = VOIP_REC;
        }
        else
        {
            peakLeft = VOIP_REC;
            peakRight = NONE;
        }
    }
    else if(scChannels == 1)
    {
        if(voipPlay)
        {
            peakLeft = VOIP_PLAY;
            peakRight = SC_MONO;
        }
        else
        {
            peakLeft = SC_MONO;
            peakRight = NONE;
        }
    }
    else
    {
        if(voipPlay)
        {
            peakLeft = VOIP_PLAY;
            peakRight = SC_MONO;
        }
        else
        {
            peakLeft = SC_LEFT;
            peakRight = SC_RIGHT;
        }
    }

    peakMeter->setChannels(peakRight == NONE ? 1 : 2);

    QRgb left = (peakLeft == VOIP_PLAY) ? qRgb(0, 80, 255) : qRgb(115, 50, 150);
    peakMeter->setColors(left, qRgb(255, 255, 0), qRgb(255, 0, 0),
                         qRgb(115, 50, 150), qRgb(255, 255, 0), qRgb(255, 0, 0));

    emit stateChanged();
}

void Stream::setPanelOpacity(int opacity)
{
    Q_UNUSED(opacity)
}

bool Stream::isActive()
{
    return (sc_process.state() != QProcess::NotRunning) || (voip_process.state() != QProcess::NotRunning);
}

Status Stream::getShoutcastStatus()
{
    return sc_status;
}

Status Stream::getVoipStatus()
{
    return voip_status;
}

void Stream::refreshState()
{
    bool streamActive = this->isActive();
    float left = 0.0f, right = 0.0f;

    if(streamActive)
    {
        switch(peakLeft)
        {
        case VOIP_PLAY: left = voip_shared ? voip_shared->playLevel/32768.0f : 0.0f; break;
        case VOIP_REC: left = voip_shared ? voip_shared->recLevel/32768.0f : 0.0f; break;
        case SC_LEFT: left = sc_shared ? sc_shared->levels[0] : 0.0f; break;
        case SC_MONO: left = sc_shared ? qMax(sc_shared->levels[0], sc_shared->levels[1]) : 0.0f; break;
        }

        switch(peakRight)
        {
        case VOIP_REC: right = voip_shared ? voip_shared->recLevel/32768.0f : 0.0f; break;
        case SC_RIGHT: right = sc_shared ? sc_shared->levels[1] : 0.0f; break;
        case SC_MONO: right = sc_shared ? qMax(sc_shared->levels[0], sc_shared->levels[1]) : 0.0f; break;
        }

        emit positionChanged(QTime(0,0).addSecs(startTime.secsTo(QDateTime::currentDateTime())));
    }

    peakMeter->setPeak(left, right);
}

template <typename T>
void Stream::createSharedMemory(QString name, T *&shared)
{
    HANDLE handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*shared), name.toUtf8().data());

    if(handle == NULL)
    {
        LOG("CreateFileMapping: " + QString::number(GetLastError()));
    }

    shared = (T*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*shared));

    if(shared == NULL)
    {
        LOG("CreateFileMapping: " + QString::number(GetLastError()));
    }
}

template <typename T>
void Stream::deleteSharedMemory(T *&shared)
{
    UnmapViewOfFile((LPCVOID)shared);
    shared = nullptr;
    //CloseHandle(handle); // TODO
}

quint32 Stream::hash_str(const char* s)
{
    const quint32 A = 54059;
    const quint32 B = 76963;
    const quint32 C = 86969;

    quint32 h = 37;
    while (*s) {
        h = (h * A) ^ (s[0] * B);
        s++;
    }
    return h;
}

void Stream::sc_start(int version, const QString &url, int port, int streamid, const QString &username, const QString &password, int channels, int bitrate, int samplerate, const QString &recDev)
{
    sc_status = RUNNING;

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
         << "--recDev=" + QString::number(this->hash_str(recDev.toUtf8().data()));

    this->createSharedMemory("sc_shared", sc_shared);
    sc_shared->end = false;

    sc_process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    sc_process.start(utils->getDataDir() + "/plugins/stream/sc.exe", args);
}

void Stream::sc_processRead()
{
    QString output = QString::fromUtf8(sc_process.readAllStandardError());
    output.replace("\r\n", "\n");
    QStringList msg = output.split('\n');
    for(int i=0; i<msg.size(); i++)
    {
        if(msg[i].size() > 0)
        {
            if(msg[i] == "Start") msg[i] = tr("Starting");
            if(msg[i] == "End") msg[i] = tr("Ended");
            if(msg[i] == "OK")
            {
                msg[i] = tr("OK");
                sc_status = OK;
            }

            bPanel->addStatus("[SHOUTcast] " + msg[i]);
        }
    }
}

void Stream::sc_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if(exitCode != 0)
    {
        bPanel->addStatus("[SHOUTcast] " + tr("Stopped with error: %1").arg(exitCode));
    }

    sc_status = STOPPED;

    this->deleteSharedMemory(sc_shared);

    emit stateChanged();
}

void Stream::voip_start(const QString &username, const QString &password, const QString &number, const QString &pin, const QString &playDev, const QString &recDev)
{
    voip_status = RUNNING;

    QStringList args;
    args << "-u" << username
         << "-p" << password
         << "-i" << number
         << "-c" << pin
         << "-d" << playDev
         << "-r" << recDev;

    this->createSharedMemory("voip_shared", voip_shared);
    voip_shared->end = false;

    voip_process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    voip_process.start(utils->getDataDir() + "/plugins/stream/voip.exe", args);
}

void Stream::voip_processRead()
{
    QString output = QString::fromUtf8(voip_process.readAllStandardError());
    output.replace("\r\n", "\n");
    QStringList msg = output.split('\n');
    for(int i=0; i<msg.size(); i++)
    {
        if(msg[i].size() > 0)
        {
            if(msg[i] == "Logging in") msg[i] = tr("Logging in");
            if(msg[i] == "Unable to log in") msg[i] = tr("Unable to log in");
            if(msg[i] == "Connecting") msg[i] = tr("Connecting");
            if(msg[i] == "Joining a conference call") msg[i] = tr("Joining a conference call");
            if(msg[i] == "Incorrect PIN") msg[i] = tr("Incorrect PIN");
            if(msg[i] == "Unable to join") msg[i] = tr("Unable to join");
            if(msg[i] == "Incorrect configuration") msg[i] = tr("Incorrect configuration");
            if(msg[i] == "OK")
            {
                voip_status = OK;
                msg[i] = tr("OK");
            }

            bPanel->addStatus("[VoIP] " + msg[i]);
        }
    }
}

void Stream::voip_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if(exitCode != 0)
    {
        bPanel->addStatus("[VoIP] " + tr("Stopped with error: %1").arg(exitCode));
    }
    else
    {
        bPanel->addStatus("[VoIP] " + tr("Ended"));
    }

    voip_status = STOPPED;

    this->deleteSharedMemory(voip_shared);

    emit stateChanged();
}

void Stream::start()
{
    settings->beginGroup(this->getPluginId());
    QString recDev = settings->value("device", "").toString();

    bool sc_enabled = settings->value("shoutcast", false).toBool();
    bool voip_enabled = settings->value("voip", false).toBool();

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

    if(voip_enabled)
    {
        this->voip_start(settings->value("voip_username").toString(),
                         QByteArray::fromBase64(settings->value("voip_password").toByteArray()),
                         settings->value("voip_number").toString(),
                         QByteArray::fromBase64(settings->value("voip_pin").toByteArray()),
                         settings->value("voip_playDev", "Mute").toString(),
                         recDev);
    }

    settings->endGroup();

    startTime = QDateTime::currentDateTime();

    emit stateChanged();
}

void Stream::stop()
{
    if(sc_shared)
        sc_shared->end = true;

    if(voip_shared)
        voip_shared->end = true;

    bPanel->addStatus(tr("Stopping stream"));

    emit stateChanged();
}
