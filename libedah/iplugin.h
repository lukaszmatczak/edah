/*
    Edah
    Copyright (C) 2016-2017  Lukasz Matczak

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

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QWidget>

class IPlugin
{
public:
    virtual QWidget *panel() = 0;
    virtual bool hasPanel() const = 0;
    virtual QWidget *getSettingsTab() = 0;
    virtual QString getPluginName() const = 0;
    virtual QString getPluginId() const = 0;

    virtual void loadSettings() = 0;
    virtual void writeSettings() = 0;
    virtual void settingsChanged() = 0;
};

Q_DECLARE_INTERFACE(IPlugin, "edah.iplugin")

#endif // IPLUGIN_H
