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

#ifndef THUMBNAILWIDGET_H
#define THUMBNAILWIDGET_H

#include "libedah.h"

#include <QLabel>
#include <QTimer>

class LIBEDAHSHARED_EXPORT ThumbnailWidget : public QLabel
{
    Q_OBJECT
public:
    ThumbnailWidget(QWidget *parent);

protected:
    void showEvent(QShowEvent *e);
    void moveEvent(QMoveEvent *e);
    void resizeEvent(QResizeEvent *e);

private:
    QTimer timer;

signals:
    void positionChanged();
};

#endif // THUMBNAILWIDGET_H
