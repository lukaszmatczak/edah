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

#ifndef WINDOWSELECTOR_H
#define WINDOWSELECTOR_H

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QTimer>

#include "windowthumbnail.h"

class WindowSelector : public QWidget
{
    Q_OBJECT
public:
    explicit WindowSelector(QWidget *mainWindow, QWidget *videoWindow, QWidget *parent = 0);
    ~WindowSelector();

protected:
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private:
    QFrame *areaFrm;
    QLabel *infoLbl;
    QFrame *borderFrm;

    QTimer updateTimer;
    WId hoverWindow;

    bool selected;
    QWidget *mainWindow;
    QWidget *videoWindow;
    WindowThumbnail *prevThumb;

    static bool scaleChkState;
    static bool mouseChkState;

signals:
    void windowSelected(WId windowID, int flags);
    void closeSignal();

public slots:
    bool close();

private slots:
    void update();
};

#endif // WINDOWSELECTOR_H
