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

#include "bluropacityeffect.h"

#include <QPainter>

QT_BEGIN_NAMESPACE
  extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

BlurOpacityEffect::BlurOpacityEffect(QObject *parent) :
    QGraphicsEffect(parent), blurRadius(0.0), opacity(0.0)
{

}

void BlurOpacityEffect::draw(QPainter *painter)
{
    painter->setOpacity(opacity);

    if(blurRadius == 0.0)
    {
        painter->drawPixmap(0, 0, this->sourcePixmap());
    }
    else
    {
        QImage img = this->sourcePixmap().toImage();
        qt_blurImage(painter, img, blurRadius, true, false);
    }
}

void BlurOpacityEffect::setBlurRadiusAndOpacity(qreal radius, qreal opacity)
{
    this->blurRadius = radius;
    this->opacity = opacity;
    this->update();
}
