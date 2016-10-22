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

#include "popup.h"
#include "bluropacityeffect.h"

#include <QTimeLine>
#include <QEventLoop>

Popup::Popup(QWidget *parent) :
    QWidget(parent), percentHeight(0.5f), percentWidth(0.5f)
{

}

void Popup::showAnimated()
{
    this->resize();

    QTimeLine timeLine(200);
    BlurOpacityEffect *blurEffect = new BlurOpacityEffect(nullptr);
    blurEffect->setBlurRadiusAndOpacity(100.0f, 0.0f);
    this->setGraphicsEffect(blurEffect);
    this->show();

    timeLine.setFrameRange(100, 0);
    connect(&timeLine, &QTimeLine::frameChanged, this, [blurEffect](int frame) {
        blurEffect->setBlurRadiusAndOpacity(frame, (100-frame)/100.0f);
    });
    timeLine.start();

    QEventLoop loop;
    connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void Popup::closeAnimated()
{
    QTimeLine timeLine(200);
    BlurOpacityEffect *blurEffect = new BlurOpacityEffect(nullptr);
    blurEffect->setBlurRadiusAndOpacity(0.0f, 1.0f);
    this->setGraphicsEffect(blurEffect);

    timeLine.setFrameRange(0, 100);
    connect(&timeLine, &QTimeLine::frameChanged, this, [blurEffect](int frame) {
        blurEffect->setBlurRadiusAndOpacity(frame, (100-frame)/100.0f);
    });
    timeLine.start();

    QEventLoop loop;
    connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();

    this->close();
}

void Popup::setSize(float width, float height)
{
    this->percentWidth = width;
    this->percentHeight = height;
}

void Popup::resize()
{
    const int width = this->parentWidget()->width()*percentWidth;
    const int height = this->parentWidget()->height()*percentHeight;

    this->setGeometry((this->parentWidget()->width()-width)/2,
                      (this->parentWidget()->height()-height)/2,
                      width,
                      height);
}
