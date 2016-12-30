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

#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H

#include <libedah/iplugin.h>

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>

class SettingsTab : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsTab(IPlugin *parent);

    void loadSettings();
    void writeSettings();

private slots:
    void sc_versionChanged(int index);

private:
    IPlugin *plugin;
    QComboBox *recDevBox;
    QGroupBox *shoutcastBox;
    QComboBox *sc_version;
    QLineEdit *sc_url;
    QLineEdit *sc_port;
    QLineEdit *sc_streamid;
    QLineEdit *sc_username;
    QLineEdit *sc_password;
    QComboBox *sc_bitrate;
    QComboBox *sc_channels;
    QComboBox *sc_sampleRate;
};

#endif // SETTINGSTAB_H
