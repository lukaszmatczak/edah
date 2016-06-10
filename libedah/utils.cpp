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

#include "utils.h"

#include <QProcessEnvironment>
#include <QWidget>
#include <QTimeLine>
#include <QGraphicsOpacityEffect>
#include <QEventLoop>
#include <QStyle>
#include <QApplication>

LIBEDAHSHARED_EXPORT Utils *utils;

Utils::Utils()
{

}

QString Utils::getLogDir()
{
#ifdef Q_OS_WIN
    return QProcessEnvironment::systemEnvironment().value("AllUsersProfile");
#endif
#ifdef Q_OS_LINUX
    return "/var/log/";
#endif
}

QString Utils::getUsername()
{
    return QProcessEnvironment::systemEnvironment()
#ifdef Q_OS_WIN
            .value("USERNAME");
#endif
#ifdef Q_OS_LINUX
            .value("USER");
#endif
}

QString Utils::getDataDir()
{
#ifdef Q_OS_WIN
    return QApplication::applicationDirPath() + "/";
#endif
#ifdef Q_OS_LINUX
    return ".."; //return "/usr/share/edah/";
#endif

}

QString Utils::getPluginPath(QString plugin)
{
#ifdef Q_OS_WIN
    return QString("%1/plugins/%2/%3.dll")
#endif
#ifdef Q_OS_LINUX
    return QString("%1/plugins/%2/lib%3.so")
#endif
            .arg(this->getDataDir())
            .arg(plugin)
            .arg(plugin);
}

void Utils::fadeInOut(QWidget *w1, QWidget *w2, int duration, int start, int stop)
{
    QTimeLine *timeLine = new QTimeLine(duration);
    QGraphicsOpacityEffect *effect1 = new QGraphicsOpacityEffect;
    QGraphicsOpacityEffect *effect2 = new QGraphicsOpacityEffect;

    effect1->setOpacity(start/255.0);
    effect2->setOpacity(start/255.0);
    w1->setGraphicsEffect(effect1);
    w2->setGraphicsEffect(effect2);

    timeLine->setFrameRange(start, stop);
    connect(timeLine, &QTimeLine::frameChanged, this, [effect1, effect2](int frame) {
        const float opacity = frame/255.0;
        effect1->setOpacity(opacity);
        effect2->setOpacity(opacity);
    });
    timeLine->start();

    QEventLoop loop;
    connect(timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void Utils::addShadowEffect(QWidget *widget, QColor color)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(30);
    effect->setColor(color);
    effect->setOffset(0,0);
    widget->setGraphicsEffect(effect);
}

void Utils::updateStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

