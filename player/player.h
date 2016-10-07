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
#include "smallpanel.h"
#include "settingstab.h"
#include "playlistmodel.h"
#include "mpv.h"
#include "videowindow.h"

#include <libedah/iplugin.h>
#include <libedah/peakmeter.h>

#include <bass.h>

#include <QObject>
#include <QDir>
#include <QRunnable>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <endpointvolume.h>
#endif

struct Song
{
    QString filename;
    QString title;
    int duration;
    qint64 mtime;
    QByteArray waveform;
};

class Player : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin")
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
    void settingsChanged();

    bool isPlaying();
    QMap<int, Song> songs;
    PlaylistModel playlistModel;

private:
    void loadSongs();
    void loadSongsInfo();
    void playFile(int n);
    void initPeakMeter(qint64 pid);

    QTranslator translator;

    BigPanel *bPanel;
    SmallPanel *sPanel;
    SettingsTab *settingsTab;

    QDir songsDir;
    PeakMeter *peakMeter;
    VideoWindow *videoWindow;
    MPV *mpv;

    HSTREAM playStream;
    bool playing;
    bool paused;
    double currPos;
    //int currNumber;
    bool autoplay;

    QTimer timer;
    QTimer peakTimer;

#ifdef Q_OS_WIN
    IAudioMeterInformation *pAudioMeterInformation;
#endif

private slots:
    void play(int entry, bool autoplay);
    void stop();
    void seek(int ms);

    void refreshState();

    void setPaused(bool paused);
    void setCurrPos(double currPos);
    void mpvEOF();

    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);

    void getPeak();

signals:
    void stateChanged(bool isPlaying);
};

#endif // PLAYER_H
