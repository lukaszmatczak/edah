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

#include "mypushbutton.h"

#include <QEvent>

MyPushButton::MyPushButton(const QString &text, QWidget *parent) :
    QPushButton(text, parent), hoverColor(0)
{
    connect(&timer, &QTimer::timeout, this, &MyPushButton::timerTimeout);
    timer.setInterval(1000/30);
}

bool MyPushButton::event(QEvent *e)
{
    if(e->type() == QEvent::TouchBegin)
    {
        e->accept();
        return true;
    }
    else if(e->type() == QEvent::Gesture)
    {
        if(this->isEnabled())
        {
            this->setStyleSheet("border-color: rgb(0,0,0);"
                                "border-bottom-color: rgb(70, 70, 70);"
                                "border-right-color: rgb(70, 70, 70);"
                                "border-width: 4 2 2 4px;");
        }

        return true;
    }
    else if(e->type() == QEvent::TouchEnd)
    {
        this->setStyleSheet("");

        return true;
    }
    else if((e->type() == QEvent::Enter) || (e->type() == QEvent::Leave))
    {
        timer.start();

        return true;
    }

    return QWidget::event(e);
}

void MyPushButton::timerTimeout()
{
    int prevColor = hoverColor;
    hoverColor = this->underMouse() ? qMin(hoverColor+8, 40)
                                    : qMax(hoverColor-2, 0);
    if(hoverColor != prevColor)
    {
        this->setStyleSheet(QString("QPushButton { background-color: rgb(36,36,%1); }")
                            .arg(36+hoverColor));
    }
    else
    {
        timer.stop();
    }
}
