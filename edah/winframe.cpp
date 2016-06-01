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

#include "winframe.h"

#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>

#include <QDebug>

const int frameWidth = 4;

bool WinFrame::onLeftBorder;
bool WinFrame::onRightBorder;
bool WinFrame::onTopBorder;
bool WinFrame::onBottomBorder;

WinFrame::WinFrame(QSize minSize, QWidget *parent) : QWidget(parent), windowFrame(nullptr)
{
    this->setMinimumSize(minSize);
    this->setMouseTracking(true);
}

void WinFrame::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void WinFrame::mouseMoveEvent(QMouseEvent *e)
{
    if(!(onLeftBorder || onRightBorder || onTopBorder || onBottomBorder))
    {
        // bottom-left corner
        if((e->pos().x() < cornerSize.width()) &&
                (e->pos().y() > this->height()-cornerSize.height()-1))
        {
            this->setCursor(Qt::SizeBDiagCursor);
        }

        // bottom-right corner
        else if((e->pos().x() > this->width()-cornerSize.width()-1) &&
                (e->pos().y() > this->height()-cornerSize.height()-1))
        {
            this->setCursor(Qt::SizeFDiagCursor);
        }

        // left and right borders
        else if((e->pos().x() < frameWidth) ||
                (e->pos().x() > this->width()-frameWidth-1))
        {
            this->setCursor(Qt::SizeHorCursor);
        }

        // top and bottom borders
        else if((e->pos().y() < frameWidth) ||
                (e->pos().y() > this->height()-frameWidth-1))
        {
            this->setCursor(Qt::SizeVerCursor);
        }
        else
        {
            this->setCursor(Qt::ArrowCursor);
        }
    }

    if(windowFrame && e->buttons() & Qt::LeftButton)
    {
        QRect newGeom = windowFrame->geometry();

        if(onLeftBorder)
        {
            QPoint pos = e->screenPos().toPoint();
            int right = windowFrame->x()+windowFrame->width();

            if(right-pos.x() < this->minimumWidth())
                pos.setX(right-this->minimumWidth());

            newGeom.setX(pos.x());
            newGeom.setWidth(right-pos.x());
        }

        if(onRightBorder)
        {
            newGeom.setWidth(e->pos().x());
        }

        if(onTopBorder)
        {
            QPoint pos = e->screenPos().toPoint();
            int bottom = windowFrame->y()+windowFrame->height();

            if(bottom-pos.y() < this->minimumHeight())
                pos.setY(bottom-this->minimumHeight());

            newGeom.setY(pos.y());
            newGeom.setHeight(bottom-pos.y());
        }

        if(onBottomBorder)
        {
            newGeom.setHeight(e->pos().y());
        }

        windowFrame->setGeometry(newGeom);
    }
}

void WinFrame::mousePressEvent(QMouseEvent *e)
{
    onLeftBorder = (e->pos().x() < frameWidth+cornerSize.width());
    onRightBorder = (e->pos().x() > this->width()-frameWidth-1-cornerSize.width());
    onTopBorder = (e->pos().y() < frameWidth);
    onBottomBorder = (e->pos().y() > this->height()-frameWidth-1-cornerSize.height());

    if(onTopBorder)
        onLeftBorder = onRightBorder = false;

    windowFrame = new WinFrame(this->minimumSize());
    windowFrame->setGeometry(this->window()->geometry());
    windowFrame->setStyleSheet(this->window()->styleSheet());
    this->setStyleSheet("border: none;");
    windowFrame->setObjectName(this->objectName() + "_active");
    windowFrame->setAttribute(Qt::WA_TranslucentBackground);
    windowFrame->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    windowFrame->show();
}

void WinFrame::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    onLeftBorder = onRightBorder = onTopBorder = onBottomBorder = false;

    emit geometryChanged(windowFrame->geometry());

    this->setStyleSheet("");
    delete windowFrame;
    windowFrame = nullptr;
}

void WinFrame::setCornerSize(QSize size)
{
    this->cornerSize = size;
}
