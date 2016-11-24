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
#include <QWidget>
#include <QTimeLine>
#include <QGraphicsOpacityEffect>
#include <QEventLoop>
#include <QStyle>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QScreen>

#include <QCryptographicHash>

#include <QDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <Shlobj.h>
#include <dwmapi.h>
#endif

Utils *utils;
LIBEDAHSHARED_EXPORT QSettings *settings;

Utils::Utils(QWidget *mainwindow) : mainwindow(mainwindow)
{
    QDir confDir(this->getConfigPath());
    if(!confDir.exists())
    {
        confDir.mkpath(".");
    }

    QFile file(QCoreApplication::applicationDirPath() + "/version.json");
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QJsonArray json = QJsonDocument::fromJson(file.readAll()).array();
        for(int i=0; i<json.size(); i++)
        {
            if(json[i].toObject()["name"] == "core")
            {
                appVersion = json[i].toObject()["v"].toString();
                appBuild   = json[i].toObject()["b"].toInt();
                break;
            }
        }
    }
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
    return QCoreApplication::applicationDirPath() + "/";
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

QString Utils::getConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString Utils::getServerUrl()
{
    return "http://edah.mn.xaa.pl/";
}

QString Utils::getAppVersion()
{
    return appVersion;
}

int Utils::getAppBuild()
{
    return appBuild;
}

QWidget *Utils::getMainWindow()
{
    return mainwindow;
}

QString Utils::parseFilename(QString fmt, const QString &name, const QDateTime &time)
{
    fmt.replace(QString("'"), QString("''"));
    fmt.replace(QString("%n%"), QString(":thisshouldntoccurinfilename:"));
    fmt.replace(QString("%%"), QString(""));
    fmt.replace(QString("%"), QString("'"));

    if(!fmt.startsWith("'"))
        fmt = "'" + fmt;

    if(!fmt.endsWith("'"))
        fmt += "'";

    QString filename = time.toString(fmt);

    if(filename.contains(":thisshouldntoccurinfilename:"))
    {
        filename.replace(QString(":thisshouldntoccurinfilename:"), name);
    }
    else
    {
        filename += " " + name;
    }

    filename.replace(QRegExp("[:\\\\\\/*?\"<>|]"), QString(""));

    return filename;
}

void Utils::fadeInOut(QWidget *w1, QWidget *w2, int duration, int start, int stop, std::function<void(int)> callback)
{
    QTimeLine timeLine(duration);
    QGraphicsOpacityEffect *effect1 = new QGraphicsOpacityEffect;
    QGraphicsOpacityEffect *effect2 = new QGraphicsOpacityEffect;

    effect1->setOpacity(start/255.0);
    effect2->setOpacity(start/255.0);
    w1->setGraphicsEffect(effect1);
    w2->setGraphicsEffect(effect2);

    timeLine.setFrameRange(start, stop);
    connect(&timeLine, &QTimeLine::frameChanged, this, [effect1, effect2, callback](int frame) {
        const float opacity = frame/255.0;
        effect1->setOpacity(opacity);
        effect2->setOpacity(opacity);
        callback(frame);
    });
    timeLine.start();

    QEventLoop loop;
    connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void Utils::addShadowEffect(QWidget *widget, QColor color)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(30);
    effect->setColor(color);
    effect->setOffset(0,0);
    widget->setGraphicsEffect(effect);
}

void Utils::updateStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString Utils::getDeviceId()
{
    BYTE machineID[255];
    DWORD machineIDsize = 255;
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    RegQueryValueEx(hKey, L"MachineGuid", NULL, NULL, machineID, &machineIDsize);
    RegCloseKey(hKey);

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray((char*)machineID, machineIDsize));
    return hash.result().toHex().left(8);
}

QString Utils::getUserId()
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(this->getUsername().toUtf8());
    return hash.result().toHex().left(8);
}

QString Utils::getFriendlyName(QString dev)
{
#ifdef Q_OS_WIN
    UINT32 PathCount, ModeCount;
    int error = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &PathCount, &ModeCount);

    if (error != ERROR_SUCCESS)
    {
        return "";
    }

    DISPLAYCONFIG_PATH_INFO *DisplayPaths = new DISPLAYCONFIG_PATH_INFO[PathCount];
    DISPLAYCONFIG_MODE_INFO *DisplayModes = new DISPLAYCONFIG_MODE_INFO[ModeCount];
    error = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &PathCount, DisplayPaths, &ModeCount, DisplayModes, NULL);

    if (error != ERROR_SUCCESS)
    {
        delete [] DisplayPaths;
        delete [] DisplayModes;
        return "";
    }

    for (UINT32 i = 0; i < PathCount; i++)
    {
        DISPLAYCONFIG_DEVICE_INFO_HEADER hdr;

        hdr.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        hdr.id = DisplayPaths[i].sourceInfo.id;
        hdr.adapterId = DisplayPaths[i].sourceInfo.adapterId;
        hdr.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);

        DISPLAYCONFIG_SOURCE_DEVICE_NAME srcInfo;
        memset(&srcInfo, 0, sizeof(srcInfo));
        srcInfo.header = hdr;

        DisplayConfigGetDeviceInfo(&srcInfo.header);

        if(!wcscmp(srcInfo.viewGdiDeviceName, dev.toStdWString().c_str()))
        {
            hdr.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            hdr.id = DisplayPaths[i].targetInfo.id;
            hdr.adapterId = DisplayPaths[i].targetInfo.adapterId;
            hdr.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);

            DISPLAYCONFIG_TARGET_DEVICE_NAME info;
            memset(&info, 0, sizeof(info));
            info.header = hdr;

            DisplayConfigGetDeviceInfo(&info.header);

            delete [] DisplayPaths;
            delete [] DisplayModes;

            QString ret = QString::fromUtf16((ushort*)info.monitorFriendlyDeviceName);

            if(this->getOutputTechnologyString(info.outputTechnology) != "")
                ret += " (" + this->getOutputTechnologyString(info.outputTechnology) + ")";

            return ret;
        }
    }

    delete [] DisplayPaths;
    delete [] DisplayModes;
    return "";
#endif
}

QString Utils::getOutputTechnologyString(int number)
{
#ifdef Q_OS_WIN
    switch(number)
    {
    case 0: return "VGA";
    case 1: return "S-Video";
    case 2: return "Composite video";
    case 3: return "Component video";
    case 4: return "DVI";
    case 5: return "HDMI";
    case 6: return "LVDS";
    case 8: return "D Video";
    case 9: return "SDI";
    case 10: return "DisplayPort";
    case 11: return "DisplayPort [internal]";
    case 12: return "UDI";
    case 13: return "UDI [internal]";
    case 14: return "SDTV";
    case 15: return "Miracast";
    case 0x80000000: return "Internal display";
    default: return "";
    }
#endif
}

void Utils::setCursorClipGeom(QRect screen)
{
    QList<QScreen*> monitors = QGuiApplication::screens();
    QRect qarea = monitors[0]->geometry();

    foreach (QScreen *monitor, monitors)
    {
        if(monitor->geometry() != screen)
        {
            qarea |= monitor->geometry();
        }
    }

    area.left = qarea.left();
    area.top = qarea.top();
    area.right = qarea.right();
    area.bottom = qarea.bottom();
}

void Utils::enableCursorClip(bool enabled)
{
    if(enabled) ClipCursor(&area);
    else ClipCursor(NULL);
}

void Utils::setExtendScreenTopology()
{
    UINT32 PathCount, ModeCount;
    int error = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &PathCount, &ModeCount);
    if (error != ERROR_SUCCESS)
    {
        return;
    }

    DISPLAYCONFIG_PATH_INFO *DisplayPaths = new DISPLAYCONFIG_PATH_INFO[PathCount];
    DISPLAYCONFIG_MODE_INFO *DisplayModes = new DISPLAYCONFIG_MODE_INFO[ModeCount];

    error = QueryDisplayConfig(QDC_DATABASE_CURRENT, &PathCount, DisplayPaths, &ModeCount, DisplayModes, &topologyId);

    SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_EXTEND);

    delete [] DisplayPaths;
    delete [] DisplayModes;
}

void Utils::setPreviousScreenTopology()
{
    if(topologyId == 0)
        return;

    SetDisplayConfig(0, NULL, 0, NULL, SDC_APPLY | topologyId);
}
