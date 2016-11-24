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

#ifndef WINDOWTHUMBNAIL_H
#define WINDOWTHUMBNAIL_H

#include <QObject>
#include <QLabel>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <dwmapi.h>
#endif

class WindowThumbnail : public QObject
{
    Q_OBJECT
public:
    explicit WindowThumbnail(WId srcID, QLabel *dest, bool withFrame, bool noScale, bool onMainwindow, QObject *parent = 0);
    virtual ~WindowThumbnail();
    void show(bool visible);
    void move(QSize srcSize);
    void setOpacity(int opacity);
    void setScale(float scale);
    QPoint mapPoint(QPoint point);
    QPixmap getCursor(QPoint *hotspot, bool forcePixmap);
    static void watchMouseMove(bool watch);

private:
    WId srcID;
    QLabel *dest;
    HTHUMBNAIL thumb;
    HWINEVENTHOOK hook;
    bool withFrame;
    bool noScale;
    QSize srcSize;
    float scale;
};

#endif // WINDOWTHUMBNAIL_H
