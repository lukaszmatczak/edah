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

class SettingsTab : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsTab(IPlugin *parent);

    void loadSettings();
    void writeSettings();

private:
    IPlugin *plugin;

    QComboBox *recDevBox;
    QLineEdit *recsDir;
    QComboBox *bitrate;
    QComboBox *channels;
    QComboBox *sampleRate;
    QComboBox *filenameFmt;
    QLabel *filenameExample;

private slots:
    void recsDirBtn_clicked();
    void filenameFmt_changed(const QString &text);
};

#endif // SETTINGSTAB_H
