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
#include <libedah/utils.h>

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QLabel>
#include <QCheckBox>

struct AudioInfo
{
    QString id;
    QString name;

    AudioInfo()
    {

    }

    AudioInfo(QString pluginId)
    {
        settings->beginGroup(pluginId);
        id = settings->value("audioDeviceId").toString();
        name = settings->value("audioDeviceName").toString();
        settings->endGroup();
    }

    void save(QString pluginId)
    {
        settings->beginGroup(pluginId);
        settings->setValue("audioDeviceId", id);
        settings->setValue("audioDeviceName", name);
        settings->endGroup();
    }

    QString toString()
    {
        return name;
    }

    inline bool operator==(const AudioInfo& x)
    {
        return (!x.id.isEmpty() && (x.id == this->id));
    }
};
Q_DECLARE_METATYPE(AudioInfo)

class SettingsTab : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsTab(IPlugin *parent);

    void loadSettings();
    void writeSettings();

    static QVector<AudioInfo> ainfo;

protected:
    void resizeEvent(QResizeEvent *e);

private:
    void setScreenLbl();
    void drawMonitors();

    IPlugin *plugin;
    QLabel *selectedScreenLbl;
    QFrame *monitorsFrame;
    QComboBox *playDevBox;
    QLineEdit *songsDir;
    QCheckBox *downloadChk;
    QLineEdit *downloadDir;
    QComboBox *downloadQuality;

    QRect selectedMonitor;

private slots:
    void songsDirBtn_clicked();
    void downloadDirBtn_clicked();
};

#endif // SETTINGSTAB_H
