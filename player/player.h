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

#ifndef PLAYER_H
#define PLAYER_H

#include "bigpanel.h"
#include "settingstab.h"

#include <libedah/iplugin.h>
#include <libedah/peakmeter.h>

#include <QObject>
#include <QDir>

#include <QMediaPlayer>
#include <QAudioProbe>

struct Song
{
    QString filename;
    QString title;
    QByteArray waveform;
};

class Player : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin" FILE "player.json")
    Q_INTERFACES(IPlugin)

public:
    Player();
    virtual ~Player();

    QWidget *bigPanel();
    QWidget *smallPanel();
    bool hasPanel() const;
    QWidget *getSettingsTab();
    QString getPluginName() const;
    QString getPluginId() const;

    void loadSettings();
    void writeSettings();

    bool isPlaying();
    QMap<int, Song> songs;

private:
    void loadSongs();

    BigPanel *bPanel;
    QWidget *smallWidget;
    SettingsTab *settingsTab;

    QDir songsDir;
    PeakMeter *peakMeter;
    QMediaPlayer *mediaPlayer;
    QAudioProbe *audioProbe;

private slots:
    void play(int number);
    void stop();

    void playerStateChanged(QMediaPlayer::State state);
    void playerPositionChanged(qint64 position);
};

#endif // PLAYER_H
