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

#include <bass.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#endif


Player::Player() : autoplay(false), currPos(0.0)
  #ifdef Q_OS_WIN
  , pAudioMeterInformation(nullptr)
  #endif
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

    BASS_SetConfig(BASS_CONFIG_UNICODE, true);

    rndPlaylist = new ShufflePlaylist(&songs);

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::play, this, &Player::play);
    connect(bPanel, &BigPanel::playSong, this, &Player::playSong);
    connect(bPanel, &BigPanel::stop, this, &Player::stop);
    connect(bPanel, &BigPanel::seek, this, &Player::seek);
    bPanel->retranslate();

    bPanel->playlistView->setModel(&playlistModel);

    connect(&playlistModel, &PlaylistModel::waveformChanged, this, [this]() {
        bPanel->posBar->setWaveform(playlistModel.getCurrentItemInfo().waveform);
    });

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);

    this->settingsChanged();

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Player::refreshState);
    timer.start();

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(0, 80, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));

    thumbnailWidget = new ThumbnailWidget(bPanel);

    videoWindow = new VideoWindow(this);
    videoThumbnail = utils->createThumbnail(videoWindow->winId(), thumbnailWidget, false, false, true);

    connect(thumbnailWidget, &ThumbnailWidget::positionChanged, this, [this]() {
        this->updateThumbnailPos();
    });

    videoWindow->setVideoThumbnail(videoThumbnail);
    videoWindow->configurationChanged();

    connect(qApp, &QGuiApplication::screenAdded, this, &Player::screenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &Player::screenRemoved);

    utils->setExtendScreenTopology();

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

    downloadManager = nullptr;
    if(settings->value("download", false).toBool())
    {
        downloadManager = new DownloadManager(settings->value("downloadDir", "").toString());
        connect(this, &Player::downloaderStart, downloadManager, &DownloadManager::start);
        downloadManager->moveToThread(&downloadThread);
        downloadThread.start();
        emit downloaderStart();
    }

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

    utils->setPreviousScreenTopology();

    videoWindow->canClose = true;
    videoWindow->close();

    downloadThread.quit();
    downloadThread.wait();

    delete mpv;
    delete videoWindow;

    delete bPanel;
    delete sPanel;
    delete settingsTab;

    delete rndPlaylist;

    BASS_Free();
}

QWidget *Player::bigPanel()
{
    sPanel->removePeakMeter(peakMeter);
    bPanel->addPeakMeter(peakMeter);

    bPanel->addThumbnail(thumbnailWidget);
    sPanel->removeThumbnail(thumbnailWidget);

    return bPanel;
}

QWidget *Player::smallPanel()
{
    bPanel->removePeakMeter(peakMeter);
    sPanel->addPeakMeter(peakMeter);

    sPanel->addThumbnail(thumbnailWidget);
    bPanel->removeThumbnail(thumbnailWidget);

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

    if(!initialized)
    {
        if(!BASS_Init(-1, 44100, 0, nullptr, nullptr))
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
    rndPlaylist->generateNewPlaylist();
}

void Player::setPanelOpacity(int opacity)
{
    utils->setThumbnailOpacity(videoThumbnail, opacity);
}

void Player::updateThumbnailPos()
{
    utils->moveThumbnail(videoThumbnail, QSize());
}

void Player::loadSongs()
{
    QFileInfoList songsListDir = songsDir.entryInfoList(QStringList() << "*.mp3" << "*.mp4", QDir::Files, QDir::Name | QDir::Reversed);
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
}

void Player::refreshState()
{
    if(videoWindow->isVisible() &&
            (!paused || videoWindow->isImageVisible() || videoWindow->isWindowVisible()) &&
            videoWindow->isFullScreen())
        utils->enableCursorClip(true);
    else
        utils->enableCursorClip(false);

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
}

bool Player::isPlaying()
{
    return currPos != 0.0;
}

void Player::setPaused(bool paused)
{
    this->paused = paused;
}

void Player::setCurrPos(double currPos)
{
    this->currPos = currPos;
    int totalPos = playlistModel.getCurrentItemInfo().duration;

    bPanel->playerPositionChanged(paused, currPos, totalPos);
    sPanel->playerPositionChanged(currPos, totalPos, autoplay);

    static bool done = false;
    if(currPos != 0.0f && !done)
    {
        done = this->initPeakMeter(mpv->getPID());
    }
}

void Player::mpvEOF()
{
    if(autoplay)
    {
        this->playSong(-1, true);
    }
    else
    {
        playlistModel.nextItem();
        bPanel->setCurrentPlaylistEntry(playlistModel.getCurrentItem());

        mpv->setPause(true);
    }
}

void Player::play(int entry)
{
    if(playlistModel.getItemInfo(entry).type == EntryInfo::Empty)
        return;

    this->autoplay = false;

    videoWindow->hideImage();
    videoWindow->hideWindow();

    if(paused && (currPos == 0.0))
    {
        bPanel->setCurrentPlaylistEntry(entry);

        if(playlistModel.getCurrentItemInfo().type == EntryInfo::Image)
        {
            mpv->stop();
            videoWindow->showImage(playlistModel.getItemInfo(entry).filename);
        }
        else if(playlistModel.getCurrentItemInfo().type == EntryInfo::Window)
        {
            mpv->stop();
            videoWindow->showWindow(playlistModel.getItemInfo(entry).winID, playlistModel.getItemInfo(entry).flags);
        }
        else
        {
            mpv->playFile(playlistModel.getItemInfo(entry).filename);
        }

        return;
    }

    mpv->pause();
}

void Player::playSong(int number, bool autoplay)
{
    this->autoplay = autoplay;

    if(autoplay && number == -1)
        number = rndPlaylist->getNext();

    videoWindow->hideImage();
    videoWindow->hideWindow();

    QString filename = songsDir.absolutePath() + "/" + songs[number].filename;

    playlistModel.setCurrentFile(filename);
    bPanel->setCurrentPlaylistEntry(-1);

    mpv->playFile(filename);
    mpv->setPause(false);
}

void Player::stop()
{
    this->autoplay = false;

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

    if(playlistModel.currFile.type == EntryInfo::AV)
        this->mpvEOF();
}

void Player::seek(int ms)
{
    mpv->seek(ms/1000);
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
    stream << song.filename << song.title /*<< song.duration*/
           << song.mtime /*<< song.waveform*/;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Song &song)
{
    stream >> song.filename >> song.title /*>> song.duration*/
           >> song.mtime /*>> song.waveform*/;

    return stream;
}

bool Player::initPeakMeter(qint64 pid)
{
#ifdef Q_OS_WIN
    //TODO: Release interfaces

    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pMMDeviceEnumerator = NULL;
    IMMDeviceCollection *pMMDeviceCollection = NULL;
    REFERENCE_TIME defaultPeriod = 100, minPeriod = 100;

    hr = CoCreateInstance(
                __uuidof(MMDeviceEnumerator), NULL,
                CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                (void**)&pMMDeviceEnumerator);
    if (FAILED(hr))
    {
        return false;
    }

    hr = pMMDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pMMDeviceCollection);
    if (FAILED(hr))
    {
        LOG(QString("IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = %1").arg(hr));
        return false;
    }

    UINT32 nDevices;
    hr = pMMDeviceCollection->GetCount(&nDevices);
    if (FAILED(hr))
    {
        LOG(QString("IMMDeviceCollection::GetCount failed: hr = %1").arg(hr));
        return false;
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
            return false;
        }

        IAudioSessionEnumerator *pAudioSessionEnumerator = NULL;
        hr = pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);
        if (FAILED(hr))
        {
            LOG(QString("IAudioSessionManager2::GetSessionEnumerator() failed: hr = %1").arg(hr));
            return false;
        }

        // iterate over all the sessions
        int count = 0;
        hr = pAudioSessionEnumerator->GetCount(&count);
        if (FAILED(hr))
        {
            LOG(QString("IAudioSessionEnumerator::GetCount() failed: hr = %1").arg(hr));
            return false;
        }

        for (int session = 0; session < count; session++)
        {
            // get the session identifier
            IAudioSessionControl *pAudioSessionControl;
            hr = pAudioSessionEnumerator->GetSession(session, &pAudioSessionControl);
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionEnumerator::GetSession() failed: hr = %1").arg(hr));
                return false;
            }

            IAudioSessionControl2 *pAudioSessionControl2;
            hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioSessionControl2));
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl::QueryInterface(IAudioSessionControl2) failed: hr = %1").arg(hr));
                return false;
            }

            DWORD processId = 0;
            hr = pAudioSessionControl2->GetProcessId(&processId);
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl2::GetProcessId() failed: hr = %1").arg(hr));
                return false;
            }

            if(processId != pid) continue;

            // get the current audio peak meter level for this session
            hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioMeterInformation));
            if (FAILED(hr))
            {
                LOG(QString("IAudioSessionControl::QueryInterface(IAudioMeterInformation) failed: hr = %1").arg(hr));
                return false;
            }

            IAudioClient *pAudioClient = NULL;
            hr = pMMDevice->Activate(
                        __uuidof(IAudioClient),
                        CLSCTX_ALL,
                        nullptr,
                        reinterpret_cast<void **>(&pAudioClient));
            if (FAILED(hr))
            {
                LOG(QString("IMMDevice::Activate(IAudioClient) failed: hr = %1").arg(hr));
                return false;
            }

            pAudioClient->GetDevicePeriod(&defaultPeriod, &minPeriod);
        }
    }

    connect(&peakTimer, &QTimer::timeout, this, &Player::getPeak);
    this->peakTimer.start(defaultPeriod/10000);

    return pAudioMeterInformation;
#endif
}

void Player::getPeak()
{
#ifdef Q_OS_WIN
    if(!pAudioMeterInformation)
        return;

    static unsigned int i;
    static float max[2];
    static int interval = (32/qMax(1, peakTimer.interval()+1)); // 30Hz
    static UINT count;
    static HRESULT ret = pAudioMeterInformation->GetMeteringChannelCount(&count); // call it only once
    static QVector<float> peaks(count);

    pAudioMeterInformation->GetChannelsPeakValues(count, peaks.data());

    for(unsigned j=0; j<count; j++)
    {
        max[j%2] = qMax(max[j%2], peaks[j]);
    }

    if(i%interval == 0)
    {
        peakMeter->setPeak(max[0], max[1]);

        max[0] = 0.0f;
        max[1] = 0.0f;
    }

    i++;
#endif
}

ShufflePlaylist::ShufflePlaylist(QMap<int, Song> *songs) : songs(songs)
{
    mtEngine = new std::mt19937(QDateTime::currentMSecsSinceEpoch());

    this->generateNewPlaylist();
}

ShufflePlaylist::~ShufflePlaylist()
{
    delete mtEngine;
}

int ShufflePlaylist::getNext()
{
    if(currPos >= playlist.size())
        generateNewPlaylist();

    if(playlist.size() < 1)
        return 0;

    return playlist[currPos++];
}

void ShufflePlaylist::generateNewPlaylist()
{
    currPos = 0;
    playlist.clear();

    auto numbers = songs->keys();
    for(int i=0; i<numbers.size(); i++)
    {
        playlist.push_back(numbers[i]);
    }

    shuffle(playlist);
}

void ShufflePlaylist::shuffle(QVector<int>& vec)
{
    if(vec.size() == 0)
        return;

    std::uniform_int_distribution<int> distribution(0, vec.size()-1);
    for(int i=0; i<vec.size(); i++)
    {
        qSwap(vec[i], vec[distribution(*mtEngine)]);
    }
}
