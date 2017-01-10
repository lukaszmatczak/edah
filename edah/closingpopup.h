/*
    Edah
    Copyright (C) 2017  Lukasz Matczak

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

#ifndef CLOSINGPOPUP_H
#define CLOSINGPOPUP_H

#include <libedah/popup.h>
#include <libedah/mypushbutton.h>

#include <QStackedWidget>

class ClosingPopup : public Popup
{
    Q_OBJECT
public:
    ClosingPopup(QWidget *parent);

protected:
    void resizeEvent(QResizeEvent *event);

private:
    void recalcSizes(const QSize &size);

    QWidget *parent;

    QStackedWidget *stacked;

    MyPushButton *shutdownBtn;
    MyPushButton *closeBtn;
    MyPushButton *cancelBtn;

private slots:
    void shutdown();
};

#endif // CLOSINGPOPUP_H
