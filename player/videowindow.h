/*
    Edah
    Copyright (C) 2015-2016  Lukasz Matczak

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

#ifndef VIDEOWINDOW_H
#define VIDEOWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QCloseEvent>
#include <QTimer>

class Player;

class VideoWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit VideoWindow(Player *player, QWidget *parent = 0);
    ~VideoWindow();

    void setVideoThumbnail(int id);

    void showImage(QString filename);
    void hideImage();
    bool isImageVisible();

    void showWindow(WId winID, int flags);
    void hideWindow();
    bool isWindowVisible();

    QWidget *videoWidget;
    bool canClose;

protected:
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    void closeEvent(QCloseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);

public slots:
    void configurationChanged();

private:
    void fadeInOut(QWidget *widget, int duration, int start, int stop);
    void fadeInOutThumbnail(int thumb, int duration, int start, int stop);

    void setFullscreenMode(QScreen *destScreen);
    void setWindowMode(QScreen *destScreen);

    int videoThumbnail;
    int windowThumbnail;

    Player *player;

    QFrame *lt, *rt, *lb, *rb;
    QLabel *imgLbl;
    QPixmap imgPix;

    QLabel *winLbl;

    bool manualFullscreen;
    QScreen *manualScreen;

    QLabel *cursor;

    QTimer thumbTimer;

signals:
    void screenNameChanged(QString name);
};

#endif // VIDEOWINDOW_H
