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
#include <Psapi.h>
#include <Shlobj.h>
#include <QtWin>
#endif

Utils *utils;
LIBEDAHSHARED_EXPORT QSettings *settings;

QMap<int, Utils::ThumbInfo> Utils::thumbInfoTable;
int Utils::currThumbInfoIdx;

HHOOK mouseMoveHook;
HWINEVENTHOOK mouseChangeHook;
bool cursorChanged;

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

    isWin10orGreater = (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS10); // TODO: wrong offset
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

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(hWinEventHook);
    Q_UNUSED(event);
    Q_UNUSED(idObject);
    Q_UNUSED(idChild);
    Q_UNUSED(dwEventThread);
    Q_UNUSED(dwmsEventTime);

    for(auto it=Utils::thumbInfoTable.begin(); it!=Utils::thumbInfoTable.end(); ++it)
    {
        if((HWND)it.value().srcID == hwnd)
        {
            utils->moveThumbnail(it.key(), QSize());
        }
    }
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

    ti.hook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
                              EVENT_OBJECT_LOCATIONCHANGE,
                              NULL,
                              WinEventProc,
                              pid,
                              tid,
                              WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

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

    QMargins offset = windows10IsTerrible((HWND)ti->srcID);

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

void Utils::setThumbnailScale(int id, float scale)
{
    if(!thumbInfoTable.contains(id))
        return;

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return;

    ti->scale = scale;
    this->moveThumbnail(id, QSize());
}

QPoint Utils::mapPointToThumbnail(int id, QPoint point)
{
    if(!thumbInfoTable.contains(id))
        return QPoint();

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return QPoint();

    if(ti->scale > 0)
    {
        // TODO: unimplemented
    }
    else if(ti->noScale)
    {
        return point - QPoint((ti->srcSize.width()-ti->dest->size().width())/2,
                              (ti->srcSize.height()-ti->dest->size().height())/2);
    }
    else
    {
        float scale = qMin((float)ti->dest->size().width()/ti->srcSize.width(),
                           (float)ti->dest->size().height()/ti->srcSize.height());

        return (point * scale) - QPoint((ti->srcSize.width()*scale-ti->dest->size().width())/2,
                                        (ti->srcSize.height()*scale-ti->dest->size().height())/2);
    }

    return QPoint();
}

QPixmap Utils::getCursorForThumbnail(int id, QPoint *hotspot, bool forcePixmap)
{
    static QImage oldCursor;
    static QPoint oldHotspot;

    if(!thumbInfoTable.contains(id))
        return QPixmap();

    ThumbInfo *ti = &thumbInfoTable[id];

    if(!ti->thumb) return QPixmap();

    if(cursorChanged)
    {
        CURSORINFO ci;
        ci.cbSize = sizeof(ci);
        GetCursorInfo(&ci);

        ICONINFO ii;
        GetIconInfo(ci.hCursor, &ii);

        QImage bmColor = QtWin::fromHBITMAP(ii.hbmColor, QtWin::HBitmapAlpha).toImage();
        QImage bmMask = QtWin::fromHBITMAP(ii.hbmMask).toImage();

        if(bmColor.isNull()) // black and white cursor
        {
            bmColor = QImage(bmMask.width(), bmMask.height()/2, QImage::Format_RGBA8888);

            for(int y=0; y<bmMask.height()/2; y++)
            {
                QRgb *color = (QRgb*)bmColor.scanLine(y);
                const QRgb *andMask = (QRgb*)bmMask.constScanLine(y);
                const QRgb *xorMask = (QRgb*)bmMask.constScanLine(y+bmMask.height()/2);
                for(int x=0; x<bmColor.width(); x++)
                {
                    ((unsigned char*)&color[x])[0] = (xorMask[x] & 0x1) ? 0xff : 0x00;
                    ((unsigned char*)&color[x])[1] = (xorMask[x] & 0x1) ? 0xff : 0x00;
                    ((unsigned char*)&color[x])[2] = (xorMask[x] & 0x1) ? 0xff : 0x00;
                    ((unsigned char*)&color[x])[3] = ((andMask[x] & 0x1) && !(xorMask[x] & 0x1)) ? 0x00 : 0xff;
                }
            }
        }
        else
        {
            for(int y=0; y<bmColor.height(); y++)
            {
                QRgb *color = (QRgb*)bmColor.scanLine(y);
                const QRgb *mask = (QRgb*)bmMask.constScanLine(y);
                for(int x=0; x<bmColor.width(); x++)
                {
                    ((unsigned char*)&color[x])[3] = ~((unsigned char*)&mask[x])[0];
                }
            }
        }

        DeleteObject(ii.hbmColor);
        DeleteObject(ii.hbmMask);

        oldHotspot.setX(ii.xHotspot);
        oldHotspot.setY(ii.yHotspot);

        oldCursor = bmColor;
    }

    float scale = qMin((float)ti->dest->size().width()/ti->srcSize.width(),
                       (float)ti->dest->size().height()/ti->srcSize.height());

    hotspot->setX(oldHotspot.x()*scale);
    hotspot->setY(oldHotspot.y()*scale);

    if(!ti->noScale)
    {
        static float oldScale;

        if(oldScale != scale)
        {
            cursorChanged = true;
            oldScale = scale;
        }

        if(cursorChanged || forcePixmap)
        {
            cursorChanged = false;
            return QPixmap::fromImage(oldCursor.scaled(oldCursor.size() * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    else if(cursorChanged || forcePixmap)
    {
        cursorChanged = false;
        return QPixmap::fromImage(oldCursor);
    }

    cursorChanged = false;
    return QPixmap();
}

void Utils::destroyThumbnail(int id)
{
    if(!thumbInfoTable.contains(id))
        return;

    DwmUnregisterThumbnail(thumbInfoTable[id].thumb);

    UnhookWinEvent(thumbInfoTable[id].hook);

    thumbInfoTable.remove(id);
}

QMargins Utils::windows10IsTerrible(HWND hwnd)
{
    if(!isWin10orGreater)
        return QMargins(0, 0, 0, 0);

    RECT rect, frame;
    GetWindowRect(hwnd, &rect);
    DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));

    return QMargins(frame.left-rect.left, frame.top-rect.top, rect.right-frame.right, rect.bottom-frame.bottom);
}

WindowInfo Utils::getWindowAt(QPoint pos, WId skipWindow)
{
    WindowInfo wi;

    HWND hwnd = GetTopWindow(NULL);
    do
    {
        if(hwnd == (HWND)skipWindow)
            continue;

        WINDOWINFO winInfo;
        GetWindowInfo(hwnd, &winInfo);

        if(!(winInfo.dwStyle & WS_VISIBLE))
            continue;

        RECT *rect = &winInfo.rcWindow;
        wi.geometry = QRect(QPoint(rect->left, rect->top), QPoint(rect->right, rect->bottom));

        if(wi.geometry.contains(pos, true))
        {
            wi.geometry -= windows10IsTerrible(hwnd);
            wi.geometry -= QMargins(0, 0, 1, 1);
            wi.windowID = (WId)hwnd;
            return wi;
        }
    } while(hwnd = GetNextWindow(hwnd, GW_HWNDNEXT));

    wi.windowID = 0;
    wi.geometry = QRect(0,0,0,0);
    return wi;
}

QString Utils::getWindowTitle(WId winID)
{
    WCHAR text[256];
    GetWindowTextW((HWND)winID, text, 256);
    if(GetLastError() == ERROR_SUCCESS)
    {
        return QString::fromUtf16((const ushort*)text);
    }
    else
    {
        return QString();
    }
}

QPixmap Utils::getWindowIcon(WId winID)
{
    QPixmap ret;
    HWND hwnd = (HWND)winID;
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    HANDLE Handle = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE,
                pid);
    if(Handle)
    {
        TCHAR Buffer[MAX_PATH];
        if (GetModuleFileNameExW(Handle, 0, Buffer, MAX_PATH))
        {
            HICON iLarge;
            SHDefExtractIconW(Buffer, 0, 0, &iLarge, NULL, MAKELONG(64, 16)); // TODO: destroy icon
            ret = QtWin::fromHICON(iLarge);
        }

        CloseHandle(Handle);
    }

    if(!ret.isNull())
        return ret;

    LRESULT iconHandle = SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if(!iconHandle)
        iconHandle = SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    if(!iconHandle)
        iconHandle = SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0);
    if(!iconHandle)
        iconHandle = GetClassLongPtr(hwnd, GCL_HICON);
    if(!iconHandle)
        iconHandle = GetClassLongPtr(hwnd, GCL_HICONSM);

    return QtWin::fromHICON((HICON)iconHandle);
}

QRect Utils::getWindowRect(WId winID)
{
    RECT rect;
    bool ok = GetWindowRect((HWND)winID, &rect);

    if(ok && !IsIconic((HWND)winID))
        return QRect(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top)
                .marginsRemoved(windows10IsTerrible((HWND)winID));
    else
        return QRect();
}

void Utils::setWindowSize(WId winID, QSize size)
{
    RECT rcWind;
    GetWindowRect((HWND)winID, &rcWind);
    QMargins border = windows10IsTerrible((HWND)winID);
    MoveWindow((HWND)winID, rcWind.left, rcWind.top, size.width()+border.left()+border.right(), size.height()+border.top()+border.bottom(), TRUE);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(wParam == WM_MOUSEMOVE)
    {
        MSLLHOOKSTRUCT *info = (MSLLHOOKSTRUCT*)lParam;
        emit utils->mouseMoved(QPoint(info->pt.x, info->pt.y));
    }

    return CallNextHookEx(mouseMoveHook, nCode, wParam, lParam);
}

void CALLBACK WinEventMouseProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(hWinEventHook);
    Q_UNUSED(event);
    Q_UNUSED(dwEventThread);
    Q_UNUSED(dwmsEventTime);

    if (hwnd == nullptr && idObject == OBJID_CURSOR && idChild == CHILDID_SELF)
    {
        cursorChanged = true;
    }
}

void Utils::watchMouseMove(bool watch)
{
    if(watch)
    {
        mouseMoveHook = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, NULL, 0);

        cursorChanged = true;
        mouseChangeHook = SetWinEventHook(EVENT_OBJECT_NAMECHANGE,
                                          EVENT_OBJECT_NAMECHANGE,
                                          NULL,
                                          WinEventMouseProc,
                                          0,
                                          0,
                                          WINEVENT_OUTOFCONTEXT);
    }
    else
    {
        UnhookWindowsHookEx(mouseMoveHook);
        UnhookWinEvent(mouseChangeHook);
    }
}
