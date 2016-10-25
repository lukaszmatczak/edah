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

#include <QDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

Utils *utils;
LIBEDAHSHARED_EXPORT QSettings *settings;

QMap<int, Utils::ThumbInfo> Utils::thumbInfoTable;
int Utils::currThumbInfoIdx;

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

int Utils::createThumbnail(WId srcID, QLabel *dest, bool withFrame, bool noScale, bool onMainwindow)
{
    ThumbInfo ti;
    ti.srcID = srcID;
    ti.dest = dest;
    ti.withFrame = withFrame;
    ti.noScale = noScale;
    ti.scale = -1.0;
    ti.srcSize = QSize();

    BOOL dwmEnabled = false;
    DwmIsCompositionEnabled(&dwmEnabled);
    if(!dwmEnabled)
    {
        dest->setText("Podgląd okna niedostępny\nNależy włączyć Aero Glass");
        return -1;
    }

    QWidget *destWindow = onMainwindow ? mainwindow : dest->window();

    HRESULT hr = DwmRegisterThumbnail((HWND)destWindow->winId(), (HWND)srcID, &ti.thumb);
    if(!SUCCEEDED(hr)) return -1;

    DWM_THUMBNAIL_PROPERTIES props;

    props.dwFlags = DWM_TNP_VISIBLE |
            DWM_TNP_RECTDESTINATION |
            DWM_TNP_SOURCECLIENTAREAONLY;

    QPoint leftTop;
    if(destWindow->children().contains(dest))
        leftTop = dest->mapTo(destWindow, QPoint(0,0));

    props.rcDestination.left = leftTop.x();
    props.rcDestination.top = leftTop.y();
    props.rcDestination.right = leftTop.x()+dest->width();
    props.rcDestination.bottom = leftTop.y()+dest->height();

    props.fVisible = TRUE;
    props.fSourceClientAreaOnly = !withFrame;
    DwmUpdateThumbnailProperties(ti.thumb, &props);

    DWORD pid;
    DWORD tid = GetWindowThreadProcessId((HWND)srcID, &pid);
/*
    ti.hook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
                              EVENT_OBJECT_LOCATIONCHANGE,
                              NULL,
                              WinEventProc,
                              pid,
                              tid,
                              WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
*/ // TODO
    int id = currThumbInfoIdx++;
    thumbInfoTable.insert(id, ti);

    this->moveThumbnail(id, QSize());

    return id;
}

void Utils::moveThumbnail(int id, QSize srcSize)
{
    if(!thumbInfoTable.contains(id))
        return;

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return;

    if(!srcSize.isValid())
    {
        SIZE size;
        if(DwmQueryThumbnailSourceSize(ti->thumb, &size) == S_OK)
            srcSize = QSize(size.cx, size.cy);
    }

    if(srcSize.isValid())
    {
        ti->srcSize = srcSize;
    }
    else
    {
        srcSize = ti->srcSize;
    }

    QSize thumbSize = ti->noScale ? srcSize : srcSize.scaled(ti->dest->size(), Qt::KeepAspectRatio);

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = 0;

    QMargins offset = QMargins(0, 0, 0, 0); //windows10IsTerrible((HWND)ti->srcID); // TODO

    if(ti->scale > 0) // if scale is set
    {
        props.dwFlags |= DWM_TNP_RECTSOURCE;
        QSize size = ti->dest->size()/ti->scale;

        props.rcSource.top = qMax(0, (srcSize.height()-size.height())/2)+offset.top();
        props.rcSource.left = qMax(0, (srcSize.width()-size.width())/2)+offset.left();
        props.rcSource.bottom = srcSize.height() - props.rcSource.top+offset.top();
        props.rcSource.right = srcSize.width() - props.rcSource.left+offset.left();
        thumbSize = QSize(props.rcSource.right-props.rcSource.left, props.rcSource.bottom-props.rcSource.top)*ti->scale;
    }
    else
    {
        props.dwFlags |= DWM_TNP_RECTSOURCE;
        props.rcSource.top = offset.top();
        props.rcSource.left = offset.left();
        props.rcSource.bottom = srcSize.height()+offset.top();
        props.rcSource.right = srcSize.width()+offset.left();
    }

    props.dwFlags |= DWM_TNP_RECTDESTINATION;

    QPoint leftTop = ti->dest->mapTo(ti->dest->window(), QPoint(0,0));
    props.rcDestination.top = leftTop.y()+(ti->dest->height()-thumbSize.height())/2;
    props.rcDestination.left = leftTop.x()+(ti->dest->width()-thumbSize.width())/2;
    props.rcDestination.right = props.rcDestination.left+thumbSize.width();
    props.rcDestination.bottom = props.rcDestination.top+thumbSize.height();

    DwmUpdateThumbnailProperties(ti->thumb, &props);
}

void Utils::showThumbnail(int id, bool visible)
{
    if(!thumbInfoTable.contains(id))
        return;

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return;

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = DWM_TNP_VISIBLE;
    props.fVisible = visible;

    DwmUpdateThumbnailProperties(ti->thumb, &props);
}

void Utils::setThumbnailOpacity(int id, int opacity)
{
    if(!thumbInfoTable.contains(id))
        return;

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return;

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = DWM_TNP_OPACITY;
    props.opacity = opacity;

    DwmUpdateThumbnailProperties(ti->thumb, &props);
}

void Utils::destroyThumbnail(int id)
{
    if(!thumbInfoTable.contains(id))
        return;

    DwmUnregisterThumbnail(thumbInfoTable[id].thumb);

    UnhookWinEvent(thumbInfoTable[id].hook);

    thumbInfoTable.remove(id);
}
