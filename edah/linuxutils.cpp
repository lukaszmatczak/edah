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

#include "linuxutils.h"

#include <QProcessEnvironment>

LinuxUtils::LinuxUtils()
{

}

QString LinuxUtils::getLogDir()
{
    return "/var/log/";
}

QString LinuxUtils::getUsername()
{
    return QProcessEnvironment::systemEnvironment().value("USER");
}

QString LinuxUtils::getDataDir()
{
    return ".."; //return "/usr/share/edah/";
}

QString LinuxUtils::getPluginPath(QString plugin)
{
    return QString("%1/plugins/%2/lib%3.so")
            .arg(this->getDataDir())
            .arg(plugin)
            .arg(plugin);
}
