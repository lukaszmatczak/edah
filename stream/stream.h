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

struct Shared
{
    bool end;
    float levels[2];
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

private:
    void start();
    void stop();
    void createSharedMemory(QString name, Shared *&shared);

    void sc_start(int version, const QString &url, int port, int streamid, const QString &username, const QString &password, int channels, int bitrate, int samplerate, const QString &recDev);

    BigPanel *bPanel;
    SmallPanel *sPanel;

    QTranslator translator;
    QTimer timer;
    SettingsTab *settingsTab;
    PeakMeter *peakMeter;

    QDateTime startTime;

    QProcess sc_process;
    int sc_channels;
    Shared *sc_shared;

private slots:
    void refreshState();

signals:
    void stateChanged();
    void positionChanged(QTime time);
};

#endif // STREAM_H
