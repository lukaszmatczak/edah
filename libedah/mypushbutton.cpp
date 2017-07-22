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

#include "mypushbutton.h"

#include <QEvent>

MyPushButton::MyPushButton(const QString &text, QWidget *parent) :
    QPushButton(text, parent), hoverColor(0)
{
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    this->grabGesture(Qt::TapGesture);
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

    return QWidget::event(e);
}

