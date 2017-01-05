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

#include "recorder.h"

#include <libedah/utils.h>
#include <libedah/logger.h>

#include <bassenc.h>

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

Recorder::Recorder(QObject *parent) :
    QObject(parent), recordingActive(false)
{
    QString localeStr = settings->value("lang", "").toString();
    if(localeStr.isEmpty())
    {
        localeStr = QLocale::system().name().left(2);
    }
    if(localeStr != "en")
    {
        if(!translator.load(QLocale(localeStr), "lang", ".", ":/recorder-lang"))
        {
            LOG(QString("Couldn't load translation for \"%1\"").arg(localeStr));
        }
    }

    qApp->installTranslator(&translator);

    BASS_SetConfig(BASS_CONFIG_UNICODE, true);

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::record, this, &Recorder::record);
    connect(bPanel, &BigPanel::stop, this, &Recorder::stop);
    bPanel->retranslate();

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);

    this->settingsChanged();

    connect(this, &Recorder::stateChanged, bPanel, &BigPanel::recorderStateChanged);
    connect(this, &Recorder::stateChanged, sPanel, &SmallPanel::recorderStateChanged);

    connect(this, &Recorder::positionChanged, bPanel, &BigPanel::recorderPositionChanged);
    connect(this, &Recorder::positionChanged, sPanel, &SmallPanel::recorderPositionChanged);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Recorder::refreshState);
    timer.start();

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(115, 50, 150), qRgb(255, 255, 0), qRgb(255, 0, 0));
}

Recorder::~Recorder()
{
    if(recordingActive)
        this->stop(bPanel->nameEdit->text());

    delete bPanel;
    delete sPanel;
    delete settingsTab;

    BASS_RecordFree();
}

QWidget *Recorder::bigPanel()
{
    sPanel->removePeakMeter(peakMeter);
    bPanel->addPeakMeter(peakMeter);

    bPanel->nameEdit->grabKeyboard();

    return bPanel;
}

QWidget *Recorder::smallPanel()
{
    bPanel->removePeakMeter(peakMeter);
    sPanel->addPeakMeter(peakMeter);

    bPanel->nameEdit->releaseKeyboard();

    return sPanel;
}

bool Recorder::hasPanel() const
{
    return true;
}

QWidget *Recorder::getSettingsTab()
{
    return settingsTab;
}

QString Recorder::getPluginName() const
{
    return tr("Recorder");
}

QString Recorder::getPluginId() const
{
    return "recorder";
}

void Recorder::loadSettings()
{
    settingsTab->loadSettings();
}

void Recorder::writeSettings()
{
    settingsTab->writeSettings();
}

BOOL CALLBACK RecordProc(HRECORD handle, const void *buffer, DWORD length, void *user)
{
    Q_UNUSED(handle)
    Q_UNUSED(buffer)
    Q_UNUSED(length)
    Q_UNUSED(user)

    return TRUE;
}

void Recorder::settingsChanged()
{
    QLocale locale = QLocale(settings->value("lang", "").toString());
    translator.load(locale, "lang", ".", ":/recorder-lang");

    settings->beginGroup(this->getPluginId());
    QString recDev = settings->value("device", "").toString();
    int channels = settings->value("channels", 1).toInt();
    int sampleRate = settings->value("sampleRate", 44100).toInt();
    settings->endGroup();

    static bool initialized = false;
    int recDevNo = -1;

    BASS_DEVICEINFO info;

    for(int i=0; BASS_RecordGetDeviceInfo(i, &info); i++)
    {
        if(recDev == info.name)
        {
            recDevNo = i;
            break;
        }
    }

    if((BASS_RecordGetDevice() != recDevNo) || !initialized)
    {
        BASS_RecordFree();

        if(!BASS_RecordInit(recDevNo))
        {
            int code = BASS_ErrorGetCode();
            QString text = QString("BASS error: %1").arg(code);
            LOG(text);

            if(code == 23)
                text += tr("\nProblem with device: \"%1\"!").arg(recDev);

            QMessageBox msg(QMessageBox::Critical, tr("Error!"), text);
            msg.exec();
        }
        else
        {
            initialized = true;

            recStream = BASS_RecordStart(sampleRate, channels, 0, RecordProc, nullptr);
        }
    }
}

void Recorder::setPanelOpacity(int opacity)
{
    Q_UNUSED(opacity)
}

bool Recorder::isRecording()
{
    return recordingActive;
}

void Recorder::refreshState()
{
    BASS_CHANNELINFO info;

    if(BASS_ChannelGetInfo(recStream, &info))
    {
        float levels[2];

        DWORD flags = info.chans == 1 ? BASS_LEVEL_MONO : BASS_LEVEL_STEREO;

        BASS_ChannelGetLevelEx(recStream, levels, 0.035f, flags);
        peakMeter->setPeak(levels[0], levels[1]);
        peakMeter->setChannels(info.chans);
    }

    if(recordingActive)
        emit positionChanged(QTime(0,0).addSecs(startTime.secsTo(QDateTime::currentDateTime())));
}

void Recorder::record()
{
    settings->beginGroup(this->getPluginId());
    QString recDir = settings->value("recsDir").toString();
    QString filenameFormat = settings->value("filenameFormat", "%n% %yyyy%-%MM%-%dd% %hh%.%mm%").toString();
    int bitrate = settings->value("bitrate", 64).toInt();
    int channels = settings->value("channels", 1).toInt();
    int sampleRate = settings->value("sampleRate", 44100).toInt();
    settings->endGroup();

    BASS_ChannelStop(recStream);

    if(!(recStream=BASS_RecordStart(sampleRate, channels, 0, RecordProc, nullptr)))
    {
        return;
    }

#ifdef Q_OS_WIN
    QString lameBin = utils->getDataDir() + "/lame.exe";
#endif
#ifdef Q_OS_LINUX
    QString lameBin = "lame";
#endif

    startTime = QDateTime::currentDateTime();
    QString filename = genNextFilename(recDir + "/" + utils->parseFilename(filenameFormat, tr("Recording"), startTime), ".mp3");

    if(!BASS_Encode_Start(recStream,
                          (wchar_t*)QString("%1 -b %2 - \"%3\"").arg(lameBin).arg(bitrate).arg(filename).utf16(),
                          BASS_UNICODE,
                          nullptr,
                          0))
    {
        LOG(QString("Cannot start recording to file \"%1\"! Error code = %2")
            .arg(filename)
            .arg(BASS_ErrorGetCode()));

        return;
    }

    recordingActive = true;
    currFile = filename;

    emit stateChanged();
}

void Recorder::stop(QString filename)
{
    settings->beginGroup(this->getPluginId());
    QString recDir = settings->value("recsDir").toString();
    QString filenameFormat = settings->value("filenameFormat", "%n% %yyyy%-%MM%-%dd% %hh%.%mm%").toString();
    int channels = settings->value("channels", 1).toInt();
    int sampleRate = settings->value("sampleRate", 44100).toInt();
    settings->endGroup();

    BASS_ChannelStop(recStream);
    BASS_Encode_StopEx(recStream, true);

    recStream = BASS_RecordStart(sampleRate, channels, 0, RecordProc, nullptr);

    if(filename.length() > 0)
    {
        QFile file(currFile);
        file.rename(genNextFilename(recDir + "/" + utils->parseFilename(filenameFormat, filename, startTime), ".mp3"));
    }

    recordingActive = false;

    emit stateChanged();
}

QString Recorder::genNextFilename(const QString &filename, const QString &ext)
{
    QFileInfo file(filename + ext);
    QDir dir(file.path());

    if(!file.exists())
        return QString("%1%2").arg(filename).arg(ext);

    QRegExp rx = QRegExp(QString("%1(\\d{4})%2")
                         .arg(QRegExp::escape(QFileInfo(filename).fileName()+"_"))
                         .arg(QRegExp::escape(ext)));

    QStringList files = dir.entryList(QDir::Files, QDir::Name | QDir::Reversed);
    files = files.filter(rx);

    if(files.isEmpty())
        return QString("%1_0002%2").arg(filename).arg(ext);

    rx.exactMatch(files[0]);
    int currNumber = rx.cap(1).toInt()+1;

    return QString("%1_%2%3").arg(filename)
            .arg(currNumber, 4, 10, QLatin1Char('0'))
            .arg(ext);
}
