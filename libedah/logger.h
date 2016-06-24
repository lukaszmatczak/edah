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

#ifndef LOGGER_H
#define LOGGER_H

#include "libedah.h"

#include <QFile>

#define LOG(msg) logger->write(QString("%1:%2: %3").arg(__FILE__).arg(__LINE__).arg(msg))

class LIBEDAHSHARED_EXPORT Logger
{
public:
    void write(const QString &msg);

private:
    Logger();

    QFile file;
    QString username;

    friend class MainWindow;
};

LIBEDAHSHARED_EXPORT extern Logger *logger;

#endif // LOGGER_H
