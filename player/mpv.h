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

#ifndef MPV_H
#define MPV_H

#include <QObject>
#include <QProcess>
#include <QWidget>

class MPV : public QObject
{
    Q_OBJECT
public:
    explicit MPV(QString audioDevId, WId videoWidgetId, bool hwdec, QObject *parent = 0);
    ~MPV();

    void playFile(QString filename);
    void stop();
    void seek(int seconds);
    void pause();
    void setPause(bool pause);

    qint64 getPID();

private:
    QProcess process;

signals:
    void processFinished();

    void currPosChanged(double currPos);
    void pauseChanged(bool paused);
    void peakLevelChanged(double peakLevel);
    void eof();

public slots:
    void mpv_readyRead();
};

#endif // MPV_H
