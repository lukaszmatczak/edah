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

#include "mpv.h"
#include "settingstab.h"

#include <libedah/utils.h>

#include <QDebug>
#include <QApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

MPV::MPV(QString audioDevId, WId videoWidgetId, bool hwdec, QObject *parent) : QObject(parent)
{
    connect(&process, &QIODevice::readyRead, this, &MPV::mpv_readyRead);
    connect(&process, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        emit processFinished();
    });
    QStringList mpvArgs;
    mpvArgs << "--force-window=yes" << "--idle" << "--input-file=/dev/stdin" << "--no-osc" << "--osd-level=0" << "--pause" << "--no-input-default-bindings";
    mpvArgs << "--audio-display=no";
    //mpvArgs << "--vo=opengl";
#ifndef Q_OS_WIN
    mpvArgs << "--af=@level:lavfi=\"astats=metadata=1:reset=4\"";
#endif
    mpvArgs << "--script=" + utils->getDataDir() + "/plugins/player/script.lua";
    mpvArgs << "--audio-device=" + audioDevId;
    mpvArgs << "--wid="+QString::number(videoWidgetId);

    if(hwdec) mpvArgs << "--hwdec=auto";

#ifdef Q_OS_WIN
    process.start(utils->getDataDir() + "/plugins/player/mpv.exe", mpvArgs);
#endif
#ifdef Q_OS_LINUX
    process.start("mpv", mpvArgs);
#endif
}

MPV::~MPV()
{
    if(process.state() != QProcess::NotRunning)
    {
        process.terminate();
        if(!process.waitForFinished(500)) process.kill();
        process.waitForFinished(500);
    }
}

qint64 MPV::getPID()
{
    return process.processId();
}

void MPV::mpv_readyRead()
{
    QStringList msgs = QString::fromUtf8(process.readAll()).split("\n");

    for(int i=0; i<msgs.size(); i++)
    {
        if(!msgs[i].startsWith("[script] ")) return;
        msgs[i].remove(0, 9);

        QJsonObject json = QJsonDocument::fromJson(msgs[i].toUtf8()).object();

        if(!json.value("currPos").isUndefined())
        {
            double currPos = json.value("currPos").toDouble();
            emit currPosChanged(currPos);
        }

        if(!json.value("pause").isUndefined())
        {
            bool paused = json.value("pause").toString() == "yes";
            emit pauseChanged(paused);
        }

        if(!json.value("event").isUndefined())
        {
            if(json.value("event").toString() == "end_file")
            {
                emit eof();
            }
        }
#ifndef Q_OS_WIN
        if(!json.value("peakLevel").isUndefined())
        {
            double peak = json.value("peakLevel").toDouble();
            emit peakLevelChanged(peak);
        }
#endif
        if(!json.value("audioDev").isUndefined())
        {
            QJsonArray arr = json.value("audioDev").toArray();
            SettingsTab::ainfo.clear();
            for(int i=0; i<arr.size(); i++)
            {
                QString name = arr[i].toObject().value("name").toString();
                QString desc = arr[i].toObject().value("description").toString();
#ifdef Q_OS_WIN
                if(name.startsWith("wasapi/"))
                {
                    AudioInfo info;
                    info.id = name;
                    info.name = desc;
                    SettingsTab::ainfo.push_back(info);
                }
#endif
#ifdef Q_OS_LINUX
                AudioInfo info;
                info.id = name;
                info.name = name.indexOf('/') != -1 ? name.left(name.indexOf('/')+1) + desc : name + "/" + desc;
                SettingsTab::ainfo.push_back(info);
#endif
            }
        }
    }
}

void MPV::playFile(QString filename)
{
    filename.replace("\\", "\\\\");
    process.write(("loadfile \"" + filename + "\"\nset pause no\n").toUtf8());
}

void MPV::stop()
{
    process.write("stop\nset pause yes\n");
}

void MPV::seek(int seconds)
{
    process.write(QString("seek " + QString::number(seconds) + " absolute\n").toUtf8());
}

void MPV::pause()
{
    process.write("cycle pause\n");
}

void MPV::setPause(bool pause)
{
    process.write(pause ? "set pause yes\n" : "set pause no\n");
}
