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

#ifndef UTILS_H
#define UTILS_H

#include "libedah.h"

#include <QObject>
#include <QSettings>
#include <QLabel>

#include <functional>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

struct WindowInfo
{
    WId windowID;
    QRect geometry;
};

class LIBEDAHSHARED_EXPORT Utils : public QObject
{
    Q_OBJECT

public:
    QString getLogDir();
    QString getUsername();
    QString getDataDir();
    QString getPluginPath(QString plugin);
    QString getConfigPath();
    QString getServerUrl();
    QString getAppVersion();
    int getAppBuild();
    QWidget *getMainWindow();
    void setExperimental(bool experimental);

    QString parseFilename(QString fmt, const QString &name, const QDateTime &time);

    void fadeInOut(QWidget *w1, QWidget *w2, int duration, int start, int stop, std::function<void(int)> callback);
    void addShadowEffect(QWidget *widget, QColor color);
    void updateStyle(QWidget *widget);

    QString getDeviceId();
    QString getUserId();

    QString getFriendlyName(QString dev);
    QString getOutputTechnologyString(int number);

    void setCursorClipGeom(QRect screen);
    void enableCursorClip(bool enabled);

    void setExtendScreenTopology();
    void setPreviousScreenTopology();

private:
    Utils(QWidget *mainwindow);

    friend class MainWindow;

    QWidget *mainwindow;

    QString appVersion;
    int appBuild;
    bool experimental;

#ifdef Q_OS_WIN
    RECT area;

    DISPLAYCONFIG_TOPOLOGY_ID topologyId;

    bool isWin10orGreater;
#endif

signals:
    void mouseMoved(QPoint pos); // TODO
};

LIBEDAHSHARED_EXPORT extern Utils *utils;
LIBEDAHSHARED_EXPORT extern QSettings *settings;

#endif // UTILS_H
