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
#include "downloadmanager.h"
#include "windowthumbnail.h"

#include <libedah/iplugin.h>
#include <libedah/peakmeter.h>
#include <libedah/thumbnailwidget.h>

#include <QObject>
#include <QDir>
#include <QRunnable>
#include <QTranslator>
#include <QThread>
#include <QSystemTrayIcon>

#ifdef Q_OS_WIN
#include <endpointvolume.h>
#endif

struct Song
{
    QString filename;
    QString title;
    //int duration;
    qint64 mtime;
    //QByteArray waveform;
};

class ShufflePlaylist : public QObject
{
    Q_OBJECT
public:
    ShufflePlaylist(QMap<int, Song> *songs);
    virtual ~ShufflePlaylist();
    int getNext();
    void generateNewPlaylist();

private:
    void shuffle(QVector<int>& vec);

    QMap<int, Song> *songs;
    QVector<int> playlist;

    std::mt19937 *mtEngine;
    int currPos;
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

    void setPanelOpacity(int opacity);

    void updateThumbnailPos();

    static QStringList getSongSymbols();
    static QMargins windows10IsTerrible(HWND hwnd);
    static WindowInfo getWindowAt(QPoint pos, const QList<WId> &skipWindows);
    static QRect getWindowRect(WId winID);
    static void setWindowSize(WId winID, QSize size);

    bool isPlaying();
    QMap<int, Song> songs;
    PlaylistModel playlistModel;
    VideoWindow *videoWindow;

private:
    void loadPlaylist(QList<MultimediaInfo> info);
    void loadSongs();
    bool initPeakMeter(qint64 pid);

    QTranslator translator;
    DownloadManager *downloadManager;
    QThread downloadThread;
    QSystemTrayIcon *trayIcon;

    BigPanel *bPanel;
    SmallPanel *sPanel;
    SettingsTab *settingsTab;

    ThumbnailWidget *thumbnailWidget;
    WindowThumbnail *videoThumbnail;

    QDir songsDir;
    QString downloadDir;
    PeakMeter *peakMeter;
    MPV *mpv;
    ShufflePlaylist *rndPlaylist;

    bool paused;
    double currPos;
    bool autoplay;

    QTimer timer;
    QTimer peakTimer;

    static bool isWin10orGreater;

#ifdef Q_OS_WIN
    IAudioMeterInformation *pAudioMeterInformation;
#endif

private slots:
    void play(int entry);
    void playSong(int number, bool autoplay);
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
    void downloaderStart();
};

#endif // PLAYER_H
