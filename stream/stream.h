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

#ifndef STREAM_H
#define STREAM_H

#include "settingstab.h"
#include "bigpanel.h"
#include "smallpanel.h"

#include <libedah/iplugin.h>
#include <libedah/peakmeter.h>

#include <QObject>
#include <QLabel>
#include <QTranslator>
#include <QDateTime>
#include <QProcess>

struct SC_Shared
{
    bool end;
    float levels[2];
};

struct VOIP_Shared
{
    bool end;
    int recLevel;
    int playLevel;
};

enum Peak
{
    VOIP_PLAY,
    VOIP_REC,
    SC_LEFT,
    SC_RIGHT,
    SC_MONO,
    NONE
};

enum Status
{
    DISABLED,
    STOPPED,
    RUNNING,
    OK
};

class Stream : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin")
    Q_INTERFACES(IPlugin)

public:
    Stream(QObject *parent = 0);
    virtual ~Stream();

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

    bool isActive();
    Status getShoutcastStatus();
    Status getVoipStatus();

private:
    void start();
    void stop();
    template <typename T> void createSharedMemory(QString name, T *&shared);
    template <typename T> void deleteSharedMemory(T *&shared);

    quint32 hash_str(const char* s);
    void sc_start(int version, const QString &url, int port, int streamid, const QString &username, const QString &password, int channels, int bitrate, int samplerate, const QString &recDev);
    void voip_start(const QString &username, const QString &password, const QString &number, const QString &pin, const QString &playDev, const QString &recDev);

    BigPanel *bPanel;
    SmallPanel *sPanel;

    QTranslator translator;
    QTimer timer;
    SettingsTab *settingsTab;
    PeakMeter *peakMeter;

    Peak peakLeft;
    Peak peakRight;

    QDateTime startTime;

    QProcess sc_process;
    int sc_channels;
    SC_Shared *sc_shared;
    Status sc_status;

    QProcess voip_process;
    VOIP_Shared *voip_shared;
    Status voip_status;

private slots:
    void refreshState();
    void sc_processRead();
    void sc_processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void voip_processRead();
    void voip_processFinished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
    void stateChanged();
    void positionChanged(QTime time);
};

#endif // STREAM_H
