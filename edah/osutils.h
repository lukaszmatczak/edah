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

#ifndef OSUTILS_H
#define OSUTILS_H

#include <QObject>

class OSUtils : public QObject
{
    Q_OBJECT
public:
    virtual QString getLogDir() = 0;
    virtual QString getUsername() = 0;
    virtual QString getDataDir() = 0;
    virtual QString getPluginPath(QString plugin) = 0;
};

extern OSUtils *utils;

#endif // OSUTILS_H
