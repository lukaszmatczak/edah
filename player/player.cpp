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

#include <QAudioDecoder>

#include <QDebug>

Player::Player()
{
    songsDir = QDir(db->value(this, "songsDir").toString());

    bPanel = new BigPanel(this);
    connect(bPanel, &BigPanel::play, this, &Player::play);
    connect(bPanel, &BigPanel::stop, this, &Player::stop);

    settingsTab = new SettingsTab(this);
    smallWidget = new QLabel(this->getPluginName());

    mediaPlayer = new QMediaPlayer(this);
    mediaPlayer->setNotifyInterval(16);
    mediaPlayer->setVolume(100);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &Player::playerStateChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::playerPositionChanged);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, bPanel, &BigPanel::playerStateChanged);

    peakMeter = new PeakMeter;
    peakMeter->setColors(qRgb(0, 80, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));
    bPanel->addPeakMeter(peakMeter);

    audioProbe = new QAudioProbe(this);
    audioProbe->setSource(mediaPlayer);
    connect(audioProbe, &QAudioProbe::audioBufferProbed, peakMeter, &PeakMeter::setPeakLevel);

    this->loadSongs();
}

Player::~Player()
{
    delete bPanel;
    delete smallWidget;
    delete settingsTab;
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

            QAudioFormat format;
            format.setChannelCount(1);
            format.setCodec("audio/x-raw");
            format.setSampleType(QAudioFormat::UnSignedInt);
            format.setSampleRate(8000);
            format.setSampleSize(8);

            QAudioDecoder *decoder = new QAudioDecoder;
            decoder->setAudioFormat(format);
            decoder->setSourceFilename(songsDir.filePath(filename));
            decoder->start();

            QByteArray samples;

            connect(decoder, &QAudioDecoder::bufferReady, this, [decoder, &samples]() {
                const QAudioBuffer &buf = decoder->read();
                const quint8 *data = buf.data<quint8>();
                samples += QByteArray((const char*)data, buf.sampleCount());
            });
            QEventLoop loop;
            connect(decoder, &QAudioDecoder::finished, &loop, &QEventLoop::quit);
            loop.exec();
// TODO: memory leak
            QByteArray form;
            int size = samples.size();
            quint8 max = std::numeric_limits<quint8>::min();
            quint8 min = std::numeric_limits<quint8>::max();
            for(int i=0; i<size; i++)
            {
                max = qMax<quint8>(max, samples[i]);
                min = qMin<quint8>(min, samples[i]);

                if(!(i % qCeil(size/1023.0f)))
                {
                    form.append(max);
                    form.append(min);

                    max = std::numeric_limits<quint8>::min();
                    min = std::numeric_limits<quint8>::max();
                }
            }

            form.append(max);
            form.append(min);

            s.waveform = form;

            decoder->stop();
            delete decoder;

            //qDebug() << decoder.position() << decoder.read().byteCount();

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

bool Player::isPlaying()
{
    return (mediaPlayer->state() == QMediaPlayer::PlayingState);
}

void Player::play(int number)
{
    qDebug() << "play" << number;
    mediaPlayer->setMedia(QUrl::fromLocalFile(songsDir.filePath(songs[number].filename)));
    mediaPlayer->play();
}

void Player::stop()
{
    qDebug() << "stop";
    mediaPlayer->stop();
}

void Player::playerStateChanged(QMediaPlayer::State state)
{
    if(state == QMediaPlayer::StoppedState)
    {
        peakMeter->stop();
    }
}

void Player::playerPositionChanged(qint64 position)
{
    bPanel->playerPositionChanged(position, mediaPlayer->duration());
}
