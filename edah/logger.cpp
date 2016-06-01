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
#include "osutils.h"

#include <QDateTime>
//TODO: repair permissions to /var/log/edah.log

Logger::Logger()
{
    file.setFileName(utils->getLogDir() + "/edah.log");
    file.open(QIODevice::Append | QIODevice::Text | QIODevice::Unbuffered);

    username = utils->getUsername();
}

void Logger::write(const QString &msg)
{
    QString toWrite = QString("[%1] [%2] %3\n")
            .arg(QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss"))
            .arg(username)
            .arg(msg);

    file.write(toWrite.toUtf8());
}
