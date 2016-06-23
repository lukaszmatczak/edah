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

#ifndef UTILS_H
#define UTILS_H

#include "libedah.h"

#include <QObject>
#include <QSettings>

class LIBEDAHSHARED_EXPORT Utils : public QObject
{
    Q_OBJECT

public:
    Utils();

    QString getLogDir();
    QString getUsername();
    QString getDataDir();
    QString getPluginPath(QString plugin);
    QString getConfigPath();

    void fadeInOut(QWidget *w1, QWidget *w2, int duration, int start, int stop);
    void addShadowEffect(QWidget *widget, QColor color);
    void updateStyle(QWidget *widget);
};

LIBEDAHSHARED_EXPORT extern Utils *utils;
LIBEDAHSHARED_EXPORT extern QSettings *settings;

#endif // UTILS_H
