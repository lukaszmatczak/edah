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

#include <QDebug>

#if defined(_MSC_VER)
#undef min
#undef max
#endif

Player::Player() : currNumber(0), autoplay(false)
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

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::play, this, &Player::play);
    connect(bPanel, &BigPanel::stop, this, &Player::stop);
    connect(bPanel, &BigPanel::seek, this, &Player::seek);
    bPanel->retranslate();

    sPanel = new SmallPanel(this);
    sPanel->retranslate();

    settingsTab = new SettingsTab(this);

    this->settingsChanged();

    BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 2); //TODO??
    BASS_SetConfig(BASS_CONFIG_BUFFER, 1000);

    connect(this, &Player::stateChanged, bPanel, &BigPanel::playerStateChanged);
    connect(this, &Player::stateChanged, sPanel, &SmallPanel::playerStateChanged);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Player::refreshState);
    timer.start();

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(0, 80, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));
}

Player::~Player()
{
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();

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
    QFileInfoList songsListDir = songsDir.entryInfoList(QStringList() << "*.mp3", QDir::Files, QDir::Name | QDir::Reversed);
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

    this->loadSongsInfo();
}

void Player::loadSongsInfo()
{
    static int progress;
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
    }
}

void Player::refreshState()
{
    bool prevPlaying = playing;
    playing = (BASS_ChannelIsActive(playStream) == BASS_ACTIVE_PLAYING);

    if(prevPlaying != playing)
    {
        emit stateChanged(playing);
    }

    float levels[2] = {0.0f, 0.0f};
    if(playing)
    {
        BASS_ChannelGetLevelEx(playStream, levels, 0.035f, BASS_LEVEL_STEREO);

        QWORD posBytes = BASS_ChannelGetPosition(playStream, BASS_POS_BYTE);
        double pos = BASS_ChannelBytes2Seconds(playStream, posBytes);

        QWORD totalBytes = BASS_ChannelGetLength(playStream, BASS_POS_BYTE);
        double total = BASS_ChannelBytes2Seconds(playStream, totalBytes);

        bPanel->playerPositionChanged(pos, total);
        sPanel->playerPositionChanged(currNumber, pos, total, autoplay);
    }

    peakMeter->setPeakStereo(levels[0], levels[1]);
}

bool Player::isPlaying()
{
    return playing;
}

void Player::play(int number, bool autoplay)
{
    currNumber = number;
    this->autoplay = autoplay;

    const QString filename = songsDir.filePath(songs[number].filename);

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
    }
}

void Player::stop()
{
    BASS_ChannelStop(playStream);
    playing = false;
    currNumber = 0;
    emit stateChanged(playing);
}

void Player::seek(int ms)
{
    QWORD bytes = BASS_ChannelSeconds2Bytes(playStream, ms/1000.0);
    BASS_ChannelSetPosition(playStream, bytes, BASS_POS_BYTE);
}

//////////////////////
/// SongInfoWorker ///
//////////////////////

SongInfoWorker::SongInfoWorker(int id, QString filepath) : number(id), filepath(filepath)
{

}

void SongInfoWorker::run()
{
    HSTREAM stream;
    QByteArray form;

    int trials = 1024; // TODO??

    while((!(stream = BASS_StreamCreateFile(FALSE,
                                            filepath.utf16(),
                                            0,
                                            0,
                                            BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_STREAM_PRESCAN | BASS_SAMPLE_FLOAT | BASS_UNICODE)))
          && trials--);

    QWORD length = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
    float *buf = new float[length/1024+1];

    for(int i=0; i<1024; i++)
    {
        int count = BASS_ChannelGetData(stream, buf, (length/1024) | BASS_DATA_FLOAT);

        qint8 maxi = std::numeric_limits<qint8>::min();
        qint8 mini = std::numeric_limits<qint8>::max();

        for(int i=0; i<count/4; i++)
        {
            maxi = qMax<qint8>(maxi, buf[i]*127);
            mini = qMin<qint8>(mini, buf[i]*127);
        }

        form.append(maxi+127);
        form.append(mini+127);
    }

    double duration = BASS_ChannelBytes2Seconds(stream, length);

    BASS_StreamFree(stream);
    delete [] buf;

    emit done(number, duration*1000, form);
}

bool SongInfoWorker::autoDelete()
{
    return true;
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
