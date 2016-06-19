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

#include <libedah/database.h>
#include <libedah/logger.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QSqlQuery>
#include <QEventLoop>
#include <QMessageBox>
#include <QUrl>

#include <QDebug>

Player::Player()
{
    songsDir = QDir(db->value(this, "songsDir").toString());

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::play, this, &Player::play);
    connect(bPanel, &BigPanel::stop, this, &Player::stop);

    settingsTab = new SettingsTab(this);
    smallWidget = new QLabel(this->getPluginName());

    QString playDev = "Default"; // TODO: get it from config
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

    if(!BASS_Init(playDevNo, 44100, 0, nullptr, nullptr))
    {
        int code = BASS_ErrorGetCode();
        QString text = "BASS error: " + QString::number(code);
        LOG(text);

        if(code == 23)
            text += QString::fromUtf8("\nProblem z urządzeniem \"") + playDev + "\"!";

        QMessageBox msg(QMessageBox::Critical, "Error!", text);
        msg.exec();
        exit(-1);
    }

    BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 2); //TODO??
    BASS_SetConfig(BASS_CONFIG_BUFFER, 1000);

    connect(this, &Player::stateChanged, bPanel, &BigPanel::playerStateChanged);

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &Player::refreshState);
    timer.start();

    /*mediaPlayer = new QMediaPlayer(this);
    mediaPlayer->setNotifyInterval(16);
    mediaPlayer->setVolume(100);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &Player::playerStateChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::playerPositionChanged);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, bPanel, &BigPanel::playerStateChanged);*/

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(0, 80, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));
    bPanel->addPeakMeter(peakMeter);

    this->loadSongs();
}

Player::~Player()
{
    delete bPanel;
    delete smallWidget;
    delete settingsTab;

    BASS_Free();
}

QWidget *Player::bigPanel()
{
    return bPanel;
}

QWidget *Player::smallPanel()
{
    return smallWidget;
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
#include <QtMath>
void Player::loadSongs()
{
    db->db.exec("CREATE TABLE IF NOT EXISTS player_songs ("
                "`id` INTEGER PRIMARY KEY,"
                "`filename` TEXT,"
                "`title` TEXT,"
                "`waveform` BINARY)");

    QSqlQuery q(db->db);
    q.exec("SELECT `id`, `filename`, `title`, `waveform` FROM `player_songs`");

    while(q.next())
    {
        Song s;
        s.filename = q.value("filename").toString();
        s.title = q.value("title").toString();
        s.waveform = q.value("waveform").toByteArray();

        //qDebug() << q.value("id").toInt() << s.filename << s.title;
        songs.insert(q.value("id").toInt(), s);
    }

    QStringList songsFilenames = songsDir.entryList(QStringList() << "*.mp3", QDir::Files, QDir::Name | QDir::Reversed);

    //db->db.transaction();

    for(int i=0; i<songsFilenames.size(); i++)
    {
        QString filename = songsFilenames[i];

        QRegExp rx(".*(\\d{3}).*");
        int pos = rx.indexIn(filename);
        if(pos == -1)
        {
            continue;
        }

        int number = rx.cap(1).toInt();

        if(!songs.contains(number))
        {
            Song s;
            s.filename = filename;

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
            qDebug() << filename << s.title;

            HSTREAM stream;
            float buf[1024];
            QByteArray form;
#ifdef Q_OS_WIN
            if(stream = BASS_StreamCreateFile(FALSE,
                                              songsDir.filePath(filename).toStdWString().c_str(),
                                              0,
                                              0,
                                              BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_STREAM_PRESCAN | BASS_SAMPLE_FLOAT | BASS_UNICODE))
#endif
#ifdef Q_OS_LINUX
            if(stream = BASS_StreamCreateFile(FALSE,
                                                  songsDir.filePath(filename).toStdString().c_str(),
                                                  0,
                                                  0,
                                                  BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_STREAM_PRESCAN | BASS_SAMPLE_FLOAT))
#endif
            {
                QWORD length = BASS_ChannelGetLength(stream, BASS_POS_BYTE);

                QVector<qint8> samples;
                samples.reserve(length/4);

                int count;
                while((count = BASS_ChannelGetData(stream, buf, 4096 | BASS_DATA_FLOAT)) != -1)
                {
                    for(int i=0; i<count/4; i++)
                        samples.push_back(buf[i]*127);
                }

                const int size = samples.size();
                qint8 max = std::numeric_limits<qint8>::min();
                qint8 min = std::numeric_limits<qint8>::max();
                for(int i=0; i<size; i++)
                {
                    max = qMax<qint8>(max, samples[i]);
                    min = qMin<qint8>(min, samples[i]);

                    if(!(i % qCeil(size/1023.0f)))
                    {
                        form.append(max+128);
                        form.append(min+128);

                        max = std::numeric_limits<qint8>::min();
                        min = std::numeric_limits<qint8>::max();
                    }
                }

                form.append(max);
                form.append(min);

                s.waveform = form;

                BASS_StreamFree(stream);
            }

            QSqlQuery q(db->db);
            q.prepare("INSERT INTO `player_songs` VALUES(:id, :filename, :title, :form)");
            q.bindValue(":id", number);
            q.bindValue(":filename", s.filename);
            q.bindValue(":title", s.title);
            q.bindValue(":form", form);
            q.exec();

            songs.insert(number, s);
        }
    }

    //db->db.commit();
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
        BASS_ChannelGetLevelEx(playStream, levels, 0.035, BASS_LEVEL_STEREO);
    peakMeter->setPeakStereo(levels[0], levels[1]);

    QWORD posBytes = BASS_ChannelGetPosition(playStream, BASS_POS_BYTE);
    double pos = BASS_ChannelBytes2Seconds(playStream, posBytes);

    QWORD totalBytes = BASS_ChannelGetLength(playStream, BASS_POS_BYTE);
    double total = BASS_ChannelBytes2Seconds(playStream, totalBytes);

    bPanel->playerPositionChanged(pos, total);
}

bool Player::isPlaying()
{
    return playing;
}

void Player::play(int number)
{
    qDebug() << "play" << number;
    const QString filename = songsDir.filePath(songs[number].filename);

#ifdef Q_OS_WIN
    if (playStream = BASS_StreamCreateFile(FALSE,
                                           filename.toStdWString().c_str(),
                                           0,
                                           0,
                                           BASS_STREAM_AUTOFREE | BASS_ASYNCFILE | BASS_SAMPLE_FLOAT | BASS_UNICODE))
#endif
#ifdef Q_OS_LINUX
    if (playStream = BASS_StreamCreateFile(FALSE,
                                           filename.toStdString().c_str(),
                                           0,
                                           0,
                                           BASS_STREAM_AUTOFREE | BASS_ASYNCFILE | BASS_SAMPLE_FLOAT))
#endif
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
    qDebug() << "stop";
    BASS_ChannelStop(playStream);
    playing = false;
    emit stateChanged(playing);
}
