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

#include "videowindow.h"
#include "player.h"

#include <libedah/utils.h>

#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QThread>
#include <QTimeLine>
#include <QGraphicsOpacityEffect>

#ifdef Q_OS_WIN
    #include <QtWin>
#endif

VideoWindow::VideoWindow(Player *player, QWidget *parent)
    : QMainWindow(parent), player(player),
      canClose(false), manualScreen(nullptr), cursor(nullptr)
{
    this->move(64,64);
    this->resize(853, 480);

    lt = new QFrame(this);
    rt = new QFrame(this);
    lb = new QFrame(this);
    rb = new QFrame(this);

    QString style = "VideoWindow { background-color: black; }\n";
    if(settings->value(player->getPluginId() + "/calibration", false).toBool())
        style += "QFrame { background-color: rgb(127,127,127); }\n";
    this->setStyleSheet(style);

    videoWidget = new QWidget(this);
    this->setCentralWidget(videoWidget);
    videoWidget->hide();

    imgLbl = new QLabel(this);
    imgLbl->setAlignment(Qt::AlignCenter);
    imgLbl->setStyleSheet("background: rgb(0,0,0);");
    imgLbl->setVisible(false);

    winLbl = new QLabel(this);
    winLbl->setAlignment(Qt::AlignCenter);
    winLbl->setStyleSheet("background: rgb(0,0,0);");
    winLbl->setVisible(false);

    QList<QScreen*> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens)
        connect(screen, &QScreen::geometryChanged, this, &VideoWindow::configurationChanged);
}

VideoWindow::~VideoWindow()
{

}

void VideoWindow::setVideoThumbnail(WindowThumbnail *thumb)
{
    this->videoThumbnail = thumb;
}

void VideoWindow::fadeInOut(QWidget *widget, int duration, int start, int stop)
{
    QTimeLine *timeLine = new QTimeLine(duration, this);

    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(start/255.0);
    widget->setGraphicsEffect(effect);

    timeLine->setFrameRange(start, stop);
    connect(timeLine, &QTimeLine::frameChanged, this, [effect](int frame){
        effect->setOpacity((qreal)frame/255);
    });
    timeLine->start();

    QEventLoop loop;
    QObject::connect(timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void VideoWindow::fadeInOutThumbnail(WindowThumbnail *thumb, int duration, int start, int stop)
{
    if(!thumb)
        return;

    QTimeLine *timeLine = new QTimeLine(duration, this);

    timeLine->setFrameRange(start, stop);
    connect(timeLine, &QTimeLine::frameChanged, this, [thumb](int frame){
        thumb->setOpacity(frame);
    });
    timeLine->start();

    QEventLoop loop;
    QObject::connect(timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void VideoWindow::showImage(QString filename)
{
    this->imgPix = QPixmap(filename);
    imgLbl->setGeometry(0,0, this->width(), this->height());
    imgLbl->setPixmap(imgPix.scaled(QSize(this->width(), this->height()), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    this->setVisible(true);
    imgLbl->setVisible(true);

    fadeInOut(imgLbl, 250, 0, 255);
}

void VideoWindow::hideImage()
{
    if(imgLbl->isVisible())
    {
        fadeInOut(imgLbl, 250, 255, 0);
        imgLbl->setVisible(false);
        this->imgPix = QPixmap();
    }
}

bool VideoWindow::isImageVisible()
{
    return !imgPix.isNull();
}

void VideoWindow::showWindow(WId winID, int flags)
{
    if(windowThumbnail)
        hideWindow();

    this->setVisible(true);

    windowThumbnail = new WindowThumbnail(winID, this->winLbl, true, !(flags & EF_WIN_SCALE), false);
    windowThumbnail->setOpacity(0);
    windowThumbnail->move(QSize());

    winLbl->setGeometry(0,0, this->width(), this->height());
    winLbl->setVisible(true);

    if(flags & EF_WIN_WITHCURSOR)
    {
        cursor = new QLabel;
        cursor->setGeometry(QRect(this->pos(), QSize(16, 16)));
        cursor->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
        if(this->isFullScreen()) cursor->setWindowFlags(cursor->windowFlags() | Qt::WindowStaysOnTopHint);
#ifdef Q_OS_WIN
        cursor->setWindowFlags(cursor->windowFlags() | Qt::Tool);
#endif
#ifdef Q_OS_LINUX
        cursor->setWindowFlags(cursor->windowFlags() | Qt::X11BypassWindowManagerHint);
#endif
        cursor->setStyleSheet("background: transparent;");
        cursor->setAttribute(Qt::WA_TranslucentBackground);
        cursor->show();

        connect(utils, &Utils::mouseMoved, this, [this, winID](QPoint pos) {
            QRect winRect = Player::getWindowRect(winID);
            QPoint hotspot;
            QPixmap pix = windowThumbnail->getCursor(&hotspot, false);
            if(!pix.isNull()) cursor->setPixmap(pix);
            QPoint posOnPreview = this->geometry().topLeft()+windowThumbnail->mapPoint(pos-winRect.topLeft())-hotspot;
            if(winRect.contains(pos) &&
                    this->geometry().intersects(QRect(posOnPreview, cursor->pixmap()->size())) &&
                    Player::getWindowAt(pos, QList<WId>()).windowID == winID)
            {
                cursor->setVisible(true);
                cursor->setGeometry(QRect(posOnPreview, cursor->pixmap()->size()) & this->geometry());

                Qt::Alignment align = 0;
                align |= (posOnPreview.x() < this->geometry().x()) ? Qt::AlignRight : Qt::AlignLeft;
                align |= (posOnPreview.y() < this->geometry().y()) ? Qt::AlignBottom : Qt::AlignTop;
                cursor->setAlignment(align);
            }
            else
            {
                cursor->setVisible(false);
            }
        });

        QPoint unused;
        QPixmap pix = windowThumbnail->getCursor(&unused, true);
        cursor->setPixmap(pix);
        windowThumbnail->watchMouseMove(true);
    }

    fadeInOutThumbnail(windowThumbnail, 250, 0, 255);
}

void VideoWindow::hideWindow()
{
    if(!windowThumbnail)
        return;

    fadeInOutThumbnail(windowThumbnail, 250, 255, 0);

    if(cursor)
    {
        disconnect(utils, &Utils::mouseMoved, this, 0);
        windowThumbnail->watchMouseMove(false);

        delete cursor;
        cursor = nullptr;
    }

    winLbl->setVisible(false);

    delete windowThumbnail;
}

bool VideoWindow::isWindowVisible()
{
    return (windowThumbnail);
}

void VideoWindow::closeEvent(QCloseEvent *e)
{
    if(!canClose)
        e->ignore();
    else
        QMainWindow::closeEvent(e);
}

void VideoWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    QList<QScreen*> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens)
        if(screen->geometry().contains(e->globalPos()))
            this->manualScreen = screen;

    this->manualFullscreen = !this->isFullScreen();
    this->configurationChanged();
}

void VideoWindow::resizeEvent(QResizeEvent *event)
{
    lt->setGeometry(0, 0, 1, 1);
    rt->setGeometry(event->size().width()-1, 0, 1, 1);
    lb->setGeometry(0, event->size().height()-1, 1, 1);
    rb->setGeometry(event->size().width()-1, event->size().height()-1, 1, 1);

    if(!imgPix.isNull())
    {
        imgLbl->setGeometry(0,0,event->size().width(), event->size().height());
        imgLbl->setPixmap(imgPix.scaled(event->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    winLbl->setGeometry(0,0, this->width(), this->height());

    if(windowThumbnail)
        windowThumbnail->move(QSize());

    if(videoThumbnail)
        videoThumbnail->move(event->size());
}

void VideoWindow::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event)
    configurationChanged();
    QThread::msleep(10);
}

void VideoWindow::showEvent(QShowEvent *e)
{
    Q_UNUSED(e)

    if(videoThumbnail)
        videoThumbnail->show(true);
}

void VideoWindow::hideEvent(QHideEvent *e)
{
    Q_UNUSED(e)

    if(videoThumbnail)
        videoThumbnail->show(false);
}

void VideoWindow::setFullscreenMode(QScreen *destScreen)
{
    if(this->geometry() == destScreen->geometry()) return; // we are already in fullscreen mode

    this->setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
    if(cursor) cursor->setWindowFlags(cursor->windowFlags() | Qt::WindowStaysOnTopHint);
#ifdef Q_OS_WIN
    QtWin::setWindowExcludedFromPeek(this, true);
    QtWin::setWindowFlip3DPolicy(this, QtWin::FlipExcludeAbove);
#endif

    this->setGeometry(destScreen->geometry());
    this->showFullScreen(); // TODO ??
#ifdef Q_OS_LINUX
    this->setWindowState(Qt::WindowFullScreen);
#endif
    QString friendlyName = utils->getFriendlyName(destScreen->name());
    friendlyName += " [" + QString::number(destScreen->size().width()) + "x" + QString::number(destScreen->size().height()) + "] ";
    emit screenNameChanged(friendlyName);

    utils->setCursorClipGeom(destScreen->geometry());
}

void VideoWindow::setWindowMode(QScreen *destScreen)
{
    if(!this->isFullScreen()) return; // we are already in window mode

    this->setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    this->setGeometry(destScreen->geometry().x()+64, destScreen->geometry().y()+64, 853, 480);
#ifdef Q_OS_LINUX
    this->setWindowState(Qt::WindowNoState);
#endif
    if(cursor) cursor->setWindowFlags(cursor->windowFlags() & ~Qt::WindowStaysOnTopHint);
#ifdef Q_OS_WIN
    QtWin::setWindowExcludedFromPeek(this, false);
    QtWin::setWindowFlip3DPolicy(this, QtWin::FlipDefault);
#endif
    emit screenNameChanged("");
}

void VideoWindow::configurationChanged()
{
    QList<QScreen*> screens = QGuiApplication::screens();

    if(screens.contains(this->manualScreen)) // use screen selected by double-click
    {
        if(this->manualFullscreen)
            setFullscreenMode(this->manualScreen);
        else
            setWindowMode(this->manualScreen);

        return;
    }

    QScreen *foundScreen = nullptr;

    if(screens.size() == 2) // ignore configuration and select secondary screen
    {
        foundScreen = screens[1];
        if(foundScreen->geometry() == QGuiApplication::primaryScreen()->geometry())
            foundScreen = screens[0];
    }
    else
    {
        QRect selectedMonitor = settings->value("displayGeometry").toRect();
        foreach(QScreen *screen, screens)
        {
            if(screen->geometry() == selectedMonitor)
                foundScreen = screen;
        }
    }

    if(foundScreen)
        setFullscreenMode(foundScreen);
    else
        setWindowMode(QGuiApplication::primaryScreen());
}
