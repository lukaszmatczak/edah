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

#ifndef RECORDER_H
#define RECORDER_H

#include "settingstab.h"
#include "bigpanel.h"
#include "smallpanel.h"

#include <libedah/iplugin.h>
#include <libedah/peakmeter.h>

#include <QObject>
#include <QLabel>
#include <QTranslator>
#include <QDateTime>

#include <bass.h>

class Recorder : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin")
    Q_INTERFACES(IPlugin)

public:
    Recorder(QObject *parent = 0);
    virtual ~Recorder();

    QWidget *bigPanel();
    QWidget *smallPanel();
    bool hasPanel() const;
    QWidget *getSettingsTab();
    QString getPluginName() const;
    QString getPluginId() const;

    void loadSettings();
    void writeSettings();
    void settingsChanged();

    bool isRecording();

private:
    void record();
    void stop(QString filename);
    QString genNextFilename(const QString &filename, const QString &ext);

    BigPanel *bPanel;
    SmallPanel *sPanel;

    QTranslator translator;
    QTimer timer;
    SettingsTab *settingsTab;
    PeakMeter *peakMeter;

    HRECORD recStream;
    bool recordingActive;
    QString currFile;
    QDateTime startTime;

private slots:
    void refreshState();

signals:
    void stateChanged();
    void positionChanged(QTime time);
};

#endif // RECORDER_H
