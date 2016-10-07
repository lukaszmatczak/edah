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

#include "player.h"

#include <libedah/logger.h>
#include <libedah/utils.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QEventLoop>
#include <QMessageBox>
#include <QUrl>
#include <QDateTime>
#include <QThreadPool>
#include <QSettings>
#include <QApplication>
#include <QScreen>

#include <QDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#endif


Player::Player() : autoplay(false), currPos(0.0)
{
    QString localeStr = settings->value("lang", "").toString();
    if(localeStr.isEmpty())
    {
        localeStr = QLocale::system().name().left(2);
    }
    if(localeStr != "en")
    {
        if(!translator.load(QLocale(localeStr), "lang", ".", ":/player-lang"))
        {
            LOG(QString("Couldn't load translation for \"%1\"").arg(localeStr));
        }
    }

    qApp->installTranslator(&translator);

    BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 2); //TODO??
    BASS_SetConfig(BASS_CONFIG_BUFFER, 1000);
    BASS_SetConfig(BASS_CONFIG_UNICODE, true);

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::play, this, &Player::play);
    connect(bPanel, &BigPanel::stop, this, &Player::stop);
    connect(bPanel, &BigPanel::seek, this, &Player::seek);
    bPanel->retranslate();

    bPanel->playlistView->setModel(&playlistModel);

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);

    this->settingsChanged();

    //connect(this, &Player::stateChanged, bPanel, &BigPanel::playerStateChanged);
    connect(this, &Player::stateChanged, sPanel, &SmallPanel::playerStateChanged);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Player::refreshState);
    timer.start();

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(0, 80, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));

    videoWindow = new VideoWindow((QLabel*)nullptr, this);
    videoWindow->configurationChanged();

    connect(qApp, &QGuiApplication::screenAdded, this, &Player::screenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &Player::screenRemoved);

    //utils->setExtendScreenTopology(); // TODO

    settings->beginGroup(this->getPluginId()); // TODO: reload settings
    QString audioDeviceId = settings->value("audioDeviceId").toString();
    WId videoWidgetId = videoWindow->videoWidget->winId();
    bool hwdec = settings->value("hwdec", false).toBool();
    mpv = new MPV(audioDeviceId, videoWidgetId, hwdec);

    //connect(mpv, &MPV::processFinished, this, [this](){this->close();});
    connect(mpv, &MPV::eof, this, &Player::mpvEOF);
    connect(mpv, &MPV::currPosChanged, this, &Player::setCurrPos);
    connect(mpv, &MPV::pauseChanged, this, &Player::setPaused);
#ifndef Q_OS_WIN
    connect(mpv, &MPV::peakLevelChanged, this, &Player::setPeakLevel);
#else
    //connect(utils, &Utils::peakLevelChanged, this, &Player::setPeakLevel);
#endif
    settings->endGroup();

    // TODO
    /*
    QStringList args = QApplication::arguments();
    if(args.count() <=1 && settings->value("autoPlaylist", false).toBool())
    {
        loadPlaylist();
    }

    if(args.count() > 1)
    {
        playlistModel.addFile(args.at(1));
    }*/
}

Player::~Player()
{
    this->stop();

    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();

    //utils->setPreviousScreenTopology(); //TODO

    delete mpv;
    delete videoWindow;

    delete bPanel;
    delete sPanel;
    delete settingsTab;

    BASS_Free();
}

QWidget *Player::bigPanel()
{
    sPanel->removePeakMeter(peakMeter);
    bPanel->addPeakMeter(peakMeter);

    return bPanel;
}

QWidget *Player::smallPanel()
{
    bPanel->removePeakMeter(peakMeter);
    sPanel->addPeakMeter(peakMeter);

    return sPanel;
}

bool Player::hasPanel() const
{
    return true;
}

QWidget *Player::getSettingsTab()
{
    return settingsTab;
}

QString Player::getPluginName() const
{
    return tr("Player");
}

QString Player::getPluginId() const
{
    return "player";
}

void Player::loadSettings()
{
    settingsTab->loadSettings();
}

void Player::writeSettings()
{
    settingsTab->writeSettings();
}

void Player::settingsChanged()
{
    QLocale locale = QLocale(settings->value("lang", "").toString());
    translator.load(locale, "lang", ".", ":/player-lang");

    settings->beginGroup(this->getPluginId());
    songsDir = QDir(settings->value("songsDir").toString());
    QString playDev = settings->value("device", "").toString();
    settings->endGroup();

    static bool initialized = false;

    int playDevNo = -1;
    BASS_DEVICEINFO info;
    for (int i=1; BASS_GetDeviceInfo(i, &info); i++)
    {
        if(playDev == info.name)
        {
            playDevNo = i;
            break;
        }
    }

    if((BASS_GetDevice() != playDevNo) || !initialized)
    {
        BASS_Free();

        if(!BASS_Init(playDevNo, 44100, 0, nullptr, nullptr))
        {
            int code = BASS_ErrorGetCode();
            QString text = "BASS error: " + QString::number(code);
            LOG(text);

            if(code == 23)
                text += tr("Invalid device \"%1\"!").arg(playDev);

            QMessageBox msg(QMessageBox::Critical, "Error!", text);
            msg.exec();
        }
        else
        {
            initialized = true;
        }
    }

    this->loadSongs();
    this->bPanel->rndPlaylist->generateNewPlaylist();
}

void Player::loadSongs()
{
/*    QFileInfoList songsListDir = songsDir.entryInfoList(QStringList() << "*.mp3", QDir::Files, QDir::Name | QDir::Reversed);
    QFileInfoList songsList;
    QVector<int> songsInDir;
    bool removed = false, added = false;

    for(int i=0; i<songsListDir.size(); i++)
    {
        QRegExp rx(".*(\\d{3}).*");
        int pos = rx.indexIn(songsListDir[i].fileName());
        if(pos == -1)
        {
            continue;
        }

        int number = rx.cap(1).toInt();

        if(!songsInDir.contains(number))
        {
            songsList.append(songsListDir[i]);
            songsInDir.append(number);
        }
    }

    QFile file(utils->getConfigPath() + "/" + this->getPluginId() + "_songs.cfg", this);
    file.open(QIODevice::ReadWrite);
    {
        QDataStream stream(&file);
        stream >> songs;
    }

    QList<int> keys = songs.keys();
    for(int i=0; i<keys.size(); i++)
    {
        if(!songsDir.exists(songs[keys[i]].filename))
        {
            songs.remove(keys[i]);
            removed = true;
        }
    }

    for(int i=0; i<songsList.size(); i++)
    {
        QString filename = songsList[i].fileName();
        qint64 mtime = songsList[i].lastModified().toMSecsSinceEpoch();

        QRegExp rx(".*(\\d{3}).*");
        int pos = rx.indexIn(filename);
        if(pos == -1)
        {
            continue;
        }

        int number = rx.cap(1).toInt();

        if(!songs.contains(number) || songs[number].mtime != mtime)
        {
            Song s;
            s.filename = filename;
            s.duration = -1;
            s.mtime = mtime;

#ifdef Q_OS_WIN
            TagLib::FileRef filetag(songsDir.filePath(filename).toStdWString().c_str());
#else
            TagLib::FileRef filetag(songsDir.filePath(filename).toStdString().c_str());
#endif
            TagLib::Tag *tag = filetag.tag();
#ifdef Q_OS_WIN
            if(tag) s.title = QString::fromUtf16((ushort*)tag->title().toCWString());
#else
            if(tag) s.title = QString::fromUtf8(tag->title().toCString(true));
#endif

            QRegExp rx("\\d{3}(-. |-|_| )(.*)");
            int pos = rx.indexIn(s.title);
            if(pos > -1)
            {
                s.title = rx.cap(2);
            }

            songs.insert(number, s);
            added = true;
        }
    }

    if(added || removed)
    {
        file.resize(0);
        QDataStream stream(&file);
        stream << songs;
    }

    this->loadSongsInfo();*/
}

void Player::loadSongsInfo()
{
/*    static int progress;
    progress = 0;

    QList<int> keys = songs.keys();
    for(int i=0; i<keys.size(); i++)
    {
        if(songs[keys[i]].duration != -1)
            continue;

        progress++;

        QString filepath = songsDir.filePath(songs[keys[i]].filename);
        SongInfoWorker *worker = new SongInfoWorker(keys[i], filepath);
        connect(worker, &SongInfoWorker::done, this, [this](int id, int duration, QByteArray waveform) {
            songs[id].duration = duration;
            songs[id].waveform = waveform;

            if(--progress <= 0)
            {
                QFile file(utils->getConfigPath() + "/" + this->getPluginId() + "_songs.cfg", this);
                file.open(QIODevice::WriteOnly);
                QDataStream stream(&file);
                stream << songs;
            }
        });

        QThreadPool::globalInstance()->start(worker);
    }*/
}

void Player::refreshState()
{
    if(videoWindow->isVisible() &&
            (!paused || videoWindow->isImageVisible() || videoWindow->isWindowVisible()) &&
            videoWindow->isFullScreen())
        {}//utils->enableCursorClip(true); //TODO
    else
        {}//utils->enableCursorClip(false); //TODO

    bool hidden = (currPos == 0.0) && paused && !videoWindow->isImageVisible() && !videoWindow->isWindowVisible();

    videoWindow->videoWidget->setHidden(hidden);
    if(!settings->value("hideScreen", true).toBool()) // TODO
    {
        videoWindow->setHidden(hidden);
    }
    else
    {
        videoWindow->setHidden(false);
    }

    bool prevPlaying = playing;
    playing = (BASS_ChannelIsActive(playStream) == BASS_ACTIVE_PLAYING);

    if(prevPlaying != playing)
    {
        emit stateChanged(playing);
    }
/*
    float levels[2] = {0.0f, 0.0f};
    if(playing)
    {
        BASS_ChannelGetLevelEx(playStream, levels, 0.035f, BASS_LEVEL_STEREO);

        QWORD posBytes = BASS_ChannelGetPosition(playStream, BASS_POS_BYTE);
        double pos = BASS_ChannelBytes2Seconds(playStream, posBytes);

        QWORD totalBytes = BASS_ChannelGetLength(playStream, BASS_POS_BYTE);
        double total = BASS_ChannelBytes2Seconds(playStream, totalBytes);

        bPanel->playerPositionChanged(pos, total);
        //sPanel->playerPositionChanged(currNumber, pos, total, autoplay); // TODO, TODO: paused
    }

    peakMeter->setPeak(levels[0], levels[1]);*/
}

bool Player::isPlaying()
{
    return playing;
}

void Player::setPaused(bool paused)
{
    this->paused = paused;
}

void Player::setCurrPos(double currPos)
{
    int totalPos = playlistModel.getCurrentItemInfo().duration;
    this->currPos = currPos;

    bPanel->playerPositionChanged(paused, currPos, totalPos);

    static bool done = false;
    if(currPos != 0.0f && !done)
    {
        this->initPeakMeter(mpv->getPID());
        done = true;
    }
}

void Player::mpvEOF()
{
    bPanel->setCurrentPlaylistEntry(playlistModel.getCurrentItem()+1);

    mpv->setPause(true);
}

void Player::play(int entry, bool autoplay)
{
    this->autoplay = autoplay;

    if(paused && (currPos == 0.0))
    {
        bPanel->setCurrentPlaylistEntry(entry);
        this->playFile(entry);
        return;
    }

    mpv->pause();

/*    const QString filename = songsDir.filePath(songs[entry].filename);

    if (playStream = BASS_StreamCreateFile(FALSE,
                                           filename.utf16(),
                                           0,
                                           0,
                                           BASS_STREAM_AUTOFREE | BASS_ASYNCFILE | BASS_SAMPLE_FLOAT | BASS_UNICODE))
    {
        playing = BASS_ChannelPlay(playStream, FALSE);
        emit stateChanged(playing);
    }
    else

    {
        LOG(QString("Can't play: error code = ") + QString::number(BASS_ErrorGetCode()));
        LOG("Can't play file " + filename);
    }*/
}

void Player::playFile(int n)
{
    videoWindow->hideImage();
    videoWindow->hideWindow();

    if(playlistModel.getCurrentItemInfo().type == EntryInfo::Image)
    {
        mpv->stop();
        videoWindow->showImage(playlistModel.getItemInfo(n).filename);
    }
    else if(playlistModel.getCurrentItemInfo().type == EntryInfo::Window)
    {
        mpv->stop();
        videoWindow->showWindow(playlistModel.getItemInfo(n).winID, playlistModel.getItemInfo(n).flags);
    }
    else
    {
        mpv->playFile(playlistModel.getItemInfo(n).filename);
    }
}

void Player::stop()
{
    if(playlistModel.getCurrentItemInfo().type == EntryInfo::Image)
    {
        videoWindow->hideImage();
        bPanel->setCurrentPlaylistEntry(playlistModel.getCurrentItem()+1);
    }
    else if(playlistModel.getCurrentItemInfo().type == EntryInfo::Window)
    {
        videoWindow->hideWindow();
        bPanel->setCurrentPlaylistEntry(playlistModel.getCurrentItem()+1);
    }
    else
    {
        mpv->stop();
    }

/*    BASS_ChannelStop(playStream);
    playing = false;
    currNumber = 0;
    emit stateChanged(playing);*/
}

void Player::seek(int ms)
{
    QWORD bytes = BASS_ChannelSeconds2Bytes(playStream, ms/1000.0);
    BASS_ChannelSetPosition(playStream, bytes, BASS_POS_BYTE);
}

void Player::screenAdded(QScreen *screen)
{
    videoWindow->configurationChanged();

    connect(screen, &QScreen::geometryChanged, videoWindow, [this](){videoWindow->configurationChanged();});
}

void Player::screenRemoved(QScreen *screen)
{
    videoWindow->configurationChanged();

    disconnect(screen, &QScreen::geometryChanged, videoWindow, 0);
}

QDataStream &operator<<(QDataStream &stream, const Song &song)
{
    stream << song.filename << song.title << song.duration
           << song.mtime << song.waveform;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Song &song)
{
    stream >> song.filename >> song.title >> song.duration
           >> song.mtime >> song.waveform;

    return stream;
}

void Player::initPeakMeter(qint64 pid)
{
#ifdef Q_OS_WIN
    //TODO: Release interfaces

    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator = NULL;
    IMMDeviceCollection *pMMDeviceCollection = NULL;

    hr = CoCreateInstance(
                __uuidof(MMDeviceEnumerator), NULL,
                CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                (void**)&pMMDeviceEnumerator);
    if (FAILED(hr))
    {
        return;
    }

    hr = pMMDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection);
    if (FAILED(hr))
    {
        LOG(QString("IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = %1").arg(hr));
        return;
    }

    UINT32 nDevices;
    hr = pMMDeviceCollection->GetCount(&nDevices);
    if (FAILED(hr))
    {
        LOG(QString("IMMDeviceCollection::GetCount failed: hr = %1").arg(hr));
        return;
    }

    for (UINT32 d = 0; d < nDevices; d++)
    {
        IMMDevice *pMMDevice = NULL;
        hr = pMMDeviceCollection->Item(d, &pMMDevice);
        if (FAILED(hr))
        {
            LOG(QString("IMMDeviceCollection::Item failed: hr = %1").arg(hr));
            continue;
        }

        // get a session enumerator
        IAudioSessionManager2 *pAudioSessionManager2 = NULL;
        hr = pMMDevice->Activate(
                    __uuidof(IAudioSessionManager2),
                    CLSCTX_ALL,
                    nullptr,
                    reinterpret_cast<void **>(&pAudioSessionManager2));
        if (FAILED(hr))
        {
            LOG(QString("IMMDevice::Activate(IAudioSessionManager2) failed: hr = %1").arg(hr));
            return;
        }

        IAudioSessionEnumerator *pAudioSessionEnumerator = NULL;
        hr = pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);
        if (FAILED(hr))
        {
            LOG(QString("IAudioSessionManager2::GetSessionEnumerator() failed: hr = %1").arg(hr));
            return;
        }

        // iterate over all the sessions
        int count = 0;
        hr = pAudioSessionEnumerator->GetCount(&count);
        if (FAILED(hr))
        {
            LOG(QString("IAudioSessionEnumerator::GetCount() failed: hr = %1").arg(hr));
            return;
        }

        for (int session = 0; session < count; session++)
        {
            // get the session identifier
            IAudioSessionControl *pAudioSessionControl;
            hr = pAudioSessionEnumerator->GetSession(session, &pAudioSessionControl);
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionEnumerator::GetSession() failed: hr = %1").arg(hr));
                return;
            }

            IAudioSessionControl2 *pAudioSessionControl2;
            hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioSessionControl2));
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl::QueryInterface(IAudioSessionControl2) failed: hr = %1").arg(hr));
                return;
            }

            DWORD processId = 0;
            hr = pAudioSessionControl2->GetProcessId(&processId);
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl2::GetProcessId() failed: hr = %1").arg(hr));
                return;
            }

            if(processId != pid) continue;

            // get the current audio peak meter level for this session
            hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioMeterInformation));
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl::QueryInterface(IAudioMeterInformation) failed: hr = %1").arg(hr));
                return;
            }
        }
    }
    // TODO: IAudioMeterInformation::GetMeteringChannelCount

    connect(&peakTimer, &QTimer::timeout, this, &Player::getPeak);
    this->peakTimer.start(100); // TODO: IAudioClient::GetDevicePeriod
#endif
}

void Player::getPeak()
{
#ifdef Q_OS_WIN
    float peak = 0.0f;
    if(pAudioMeterInformation)
    {
        HRESULT hr = pAudioMeterInformation->GetPeakValue(&peak);
        if (FAILED(hr)) {
            LOG(QString("AudioMeterInformation::GetPeakValue() failed: hr = %1").arg(hr));
        }
    }

    peakMeter->setPeak(peak, 0);
#endif
}
