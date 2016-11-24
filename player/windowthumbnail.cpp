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

#include "windowthumbnail.h"
#include "player.h"

#include <libedah/utils.h>

#ifdef Q_OS_WIN
#include <QtWin>
#endif

QMap<HWINEVENTHOOK, WindowThumbnail*> thumbs;
HHOOK mouseMoveHook;
HWINEVENTHOOK mouseChangeHook;
bool cursorChanged;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(event)
    Q_UNUSED(hwnd)
    Q_UNUSED(idObject)
    Q_UNUSED(idChild)
    Q_UNUSED(dwEventThread)
    Q_UNUSED(dwmsEventTime)

    thumbs[hWinEventHook]->move(QSize());
}

WindowThumbnail::WindowThumbnail(WId srcID, QLabel *dest, bool withFrame, bool noScale, bool onMainwindow, QObject *parent)
    : QObject(parent), srcID(srcID), dest(dest), withFrame(withFrame), noScale(noScale), scale(-1.0f)
{
    BOOL dwmEnabled = false;
    DwmIsCompositionEnabled(&dwmEnabled);
    if(!dwmEnabled)
    {
        dest->setText("Preview unavailable");
        return;
    }

    QWidget *destWindow = onMainwindow ? utils->getMainWindow() : dest->window();

    HRESULT hr = DwmRegisterThumbnail((HWND)destWindow->winId(), (HWND)srcID, &this->thumb);
    if(!SUCCEEDED(hr))
    {
        this->thumb = 0;
        return;
    }

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
    DwmUpdateThumbnailProperties(this->thumb, &props);

    DWORD pid;
    DWORD tid = GetWindowThreadProcessId((HWND)srcID, &pid);

    this->hook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
                                 EVENT_OBJECT_LOCATIONCHANGE,
                                 NULL,
                                 WinEventProc,
                                 pid,
                                 tid,
                                 WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    thumbs[this->hook] = this;

    this->move(QSize());
}

WindowThumbnail::~WindowThumbnail()
{
    if(!this->thumb)
        return;

    DwmUnregisterThumbnail(this->thumb);
    UnhookWinEvent(this->hook);
    thumbs.remove(this->hook);
}

void WindowThumbnail::move(QSize srcSize)
{
    if(!this->thumb)
        return;

    if(!srcSize.isValid())
    {
        SIZE size;
        if(DwmQueryThumbnailSourceSize(this->thumb, &size) == S_OK)
            srcSize = QSize(size.cx, size.cy);
    }

    if(srcSize.isValid())
    {
        this->srcSize = srcSize;
    }
    else
    {
        srcSize = this->srcSize;
    }

    QSize thumbSize = this->noScale ? srcSize : srcSize.scaled(this->dest->size(), Qt::KeepAspectRatio);

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = 0;

    QMargins offset = Player::windows10IsTerrible((HWND)this->srcID);

    if(this->scale > 0) // if scale is set
    {
        props.dwFlags |= DWM_TNP_RECTSOURCE;
        QSize size = this->dest->size()/this->scale;

        props.rcSource.top = qMax(0, (srcSize.height()-size.height())/2)+offset.top();
        props.rcSource.left = qMax(0, (srcSize.width()-size.width())/2)+offset.left();
        props.rcSource.bottom = srcSize.height() - props.rcSource.top+offset.top();
        props.rcSource.right = srcSize.width() - props.rcSource.left+offset.left();
        thumbSize = QSize(props.rcSource.right-props.rcSource.left, props.rcSource.bottom-props.rcSource.top)*this->scale;
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

    QPoint leftTop = this->dest->mapTo(this->dest->window(), QPoint(0,0));
    props.rcDestination.top = leftTop.y()+(this->dest->height()-thumbSize.height())/2;
    props.rcDestination.left = leftTop.x()+(this->dest->width()-thumbSize.width())/2;
    props.rcDestination.right = props.rcDestination.left+thumbSize.width();
    props.rcDestination.bottom = props.rcDestination.top+thumbSize.height();

    DwmUpdateThumbnailProperties(this->thumb, &props);
}

void WindowThumbnail::show(bool visible)
{
    if(!this->thumb)
        return;

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = DWM_TNP_VISIBLE;
    props.fVisible = visible;

    DwmUpdateThumbnailProperties(this->thumb, &props);
}

void WindowThumbnail::setOpacity(int opacity)
{
    if(!this->thumb)
        return;

    DWM_THUMBNAIL_PROPERTIES props;
    props.dwFlags = DWM_TNP_OPACITY;
    props.opacity = opacity;

    DwmUpdateThumbnailProperties(this->thumb, &props);
}

void WindowThumbnail::setScale(float scale)
{
    if(!this->thumb)
        return;

    this->scale = scale;
    this->move(QSize());
}

QPoint WindowThumbnail::mapPoint(QPoint point)
{
    if(!this->thumb)
        return QPoint();

    if(this->scale > 0)
    {
        // TODO: unimplemented
    }
    else if(this->noScale)
    {
        return point - QPoint((this->srcSize.width()-this->dest->size().width())/2,
                              (this->srcSize.height()-this->dest->size().height())/2);
    }
    else
    {
        float scale = qMin((float)this->dest->size().width()/this->srcSize.width(),
                           (float)this->dest->size().height()/this->srcSize.height());

        return (point * scale) - QPoint((this->srcSize.width()*scale-this->dest->size().width())/2,
                                        (this->srcSize.height()*scale-this->dest->size().height())/2);
    }

    return QPoint();
}

QPixmap WindowThumbnail::getCursor(QPoint *hotspot, bool forcePixmap)
{
    static QImage oldCursor;
    static QPoint oldHotspot;

    if(!this->thumb)
        return QPixmap();

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

    float scale = qMin((float)this->dest->size().width()/this->srcSize.width(),
                       (float)this->dest->size().height()/this->srcSize.height());

    hotspot->setX(oldHotspot.x()*scale);
    hotspot->setY(oldHotspot.y()*scale);

    if(!this->noScale)
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
    Q_UNUSED(hWinEventHook)
    Q_UNUSED(event)
    Q_UNUSED(dwEventThread)
    Q_UNUSED(dwmsEventTime)

    if (hwnd == nullptr && idObject == OBJID_CURSOR && idChild == CHILDID_SELF)
    {
        cursorChanged = true;
    }
}

void WindowThumbnail::watchMouseMove(bool watch)
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
