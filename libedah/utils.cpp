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

Utils *utils;

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

