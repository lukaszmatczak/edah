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

#include "logger.h"
#include "utils.h"

#include <QDateTime>
//TODO: repair permissions to /var/log/edah.log

LIBEDAHSHARED_EXPORT Logger *logger;

/*!
 \class Logger
 \inmodule Edah
 \brief The Logger class provides access to log file.

 Instance of this class is created automatically by core module as global object:
 \code
 Logger *logger;
 \endcode

 Log file is stored in "%ALLUSERSPROFILE%\\edah.log" on Windows
 and "/var/log/edah.log" on Linux.
 */

/*!
 \macro void LOG(const QString &msg)
 \relates Logger
 Writes \a msg to log file. The entry has following format:
 \code
 [yy-MM-dd hh:mm:ss] [username] __FILE__:__LINE__: msg
 \endcode

 For example
 \code
 LOG("Cannot load plugin \"player\"!");
 \endcode
 writes to log file
 \code
 [16-06-09 11:24:48] [lukasz] player/player.cpp:24: Cannot load plugin player!
 \endcode
 */

/*!
 \fn Logger::Logger()
 Don't create this object manually. Core module creates it automatically.
 */
Logger::Logger()
{
    file.setFileName(utils->getLogDir() + "/edah.log");
    file.open(QIODevice::Append | QIODevice::Text | QIODevice::Unbuffered);

    username = utils->getUsername();
}

/*!
 \fn void Logger::write(const QString &msg)
 Don't call this method explicitly. Use LOG() macro instead.
 */
void Logger::write(const QString &msg)
{
    QString toWrite = QString("[%1] [%2] %3\n")
            .arg(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"))
            .arg(username)
            .arg(msg);

    file.write(toWrite.toUtf8());
}
