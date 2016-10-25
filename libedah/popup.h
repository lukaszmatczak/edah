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

#ifndef POPUP_H
#define POPUP_H

#include "libedah.h"

#include <QDialog>

class LIBEDAHSHARED_EXPORT Popup : public QDialog
{
    Q_OBJECT
public:
    explicit Popup(QWidget *parent);
    void setSize(float width, float height);
    int exec();
    void setStyleSheet(const QString &stylesheet);

protected:
    void paintEvent(QPaintEvent *e);
    void closeEvent(QCloseEvent *e);

public slots:
    void resize();

private:
    void showAnimated();

    QWidget *parent;
    QString stylesheet;

    float percentWidth;
    float percentHeight;
};

#endif // POPUP_H
