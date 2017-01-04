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

#include "splashscreen.h"

#include <QPainter>

SplashScreen::SplashScreen(QPixmap &pixmap) : QSplashScreen(pixmap), curr(0), max(1)
{

}

void SplashScreen::drawContents(QPainter *painter)
{
    painter->setBrush(QColor(0, 0x50, 0xff));
    painter->drawRect(0, 115, 300.0f/max*curr, 5);
}

void SplashScreen::setProgress(int curr, int max)
{
    this->curr = curr;
    this->max = max;

    this->repaint();
}
