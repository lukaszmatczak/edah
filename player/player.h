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

#include <QObject>
#include <QDir>

#include <QMediaPlayer>
#include <QAudioProbe>

struct Song
{
    QString filename;
    QString title;
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

    template <typename T>
    float calcMax(const QAudioBuffer &buffer, int channel)
    {
        const T *data = buffer.data<T>();
        T ret = 0;

        const int count = buffer.frameCount();
        const int channelCount = buffer.format().channelCount();

        for(int i=0; i<count-channelCount; i+=channelCount)
        {
            ret = qMax(ret, data[i+channel]);
        }

        if(std::is_same<T, float>::value)
        {
            return ret;
        }

        return ret / (float)std::numeric_limits<T>::max();
    }

    BigPanel *bPanel;
    QWidget *smallWidget;
    SettingsTab *settingsTab;

    QDir songsDir;
    QMediaPlayer *mediaPlayer;
    QAudioProbe *audioProbe;

private slots:
    void play(int number);
    void stop();

    void calcPeak(QAudioBuffer buffer);
};

#endif // PLAYER_H
