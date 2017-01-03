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

#include "peakmeter.h"

#include "logger.h"

#include <QPainter>
#include <QtMath>
#include <QLayout>
#include <QDebug>

PeakMeter::PeakMeter(QWidget *parent) : QWidget(parent), channels(2)
{
    for(int i=0; i<2; i++)
    {
        peaks[i] = speed[i] = 0.0f;
        max[i] = -1.0f;
    }

    this->setColors(qRgb(0, 0, 255), qRgb(255, 255, 0), qRgb(255, 0, 0));

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &PeakMeter::timerTimeout);
    timer.start();
}

PeakMeter::~PeakMeter()
{

}

void PeakMeter::setPeak(float left, float right)
{
    peaks[0] = qMax(peaks[0], left);
    peaks[1] = qMax(peaks[1], right);
}

void PeakMeter::setChannels(int count)
{
    this->channels = qMin(count, 2);
}

QRgb PeakMeter::blendColors(QRgb color1, QRgb color2, float r)
{
    return qRgb(qRed(color1)*(1-r) + qRed(color2)*r,
                qGreen(color1)*(1-r) + qGreen(color2)*r,
                qBlue(color1)*(1-r) + qBlue(color2)*r);
}

void PeakMeter::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter p(this);

    int margin = 0;

    float width = (250.0*channels-50.0);

    if(this->height()/this->width() < 1580.0/width)
    {
        int destWidth = this->height()*width/1580.0;
        margin = (this->width()-destWidth)/2;
    }

    p.setRenderHints(QPainter::Antialiasing);
    qreal hScale = (this->width()-margin*2)/width;
    p.scale(hScale, this->height()/1580.0);

    QRadialGradient gradient(0.5, 0.5, 0.5);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    for(int channel=0; channel<this->channels; channel++)
    {
        for(int i=0; i<32; i++)
        {
            QColor color = QColor::fromRgb(channel ? rightColors[i] : leftColors[i]);

            float maxPos = max[channel]*31-i;
            if(i/32.0 > peaks[channel] &&
                    ((maxPos > 0.5f) || (maxPos < -0.5f+std::numeric_limits<float>::epsilon())))
            {
                color = color.lighter(8);
            }

            gradient.setColorAt(0, color.lighter(120));
            gradient.setColorAt(1, color);
            p.setBrush(QBrush(gradient));
            p.setPen(color);
            p.drawRoundedRect((channel*250)+margin/hScale, 1580-(i*50), 200, 30, 10, 100);
        }
    }
}

void PeakMeter::setColors(QRgb low, QRgb mid, QRgb high)
{
    for(int i=0; i<23; i++)
    {
        leftColors[i] = rightColors[i] = blendColors(low, mid, qMax(0,i-17)/6.0f);
    }
    for(int i=23; i<32; i++)
    {
        leftColors[i] = rightColors[i] = blendColors(mid, high, qMax(0,i-28)/3.0f);
    }
}

void PeakMeter::setColors(QRgb leftLow, QRgb leftMid, QRgb leftHigh,
                          QRgb rightLow, QRgb rightMid, QRgb rightHigh)
{
    for(int i=0; i<23; i++)
    {
        leftColors[i] = blendColors(leftLow, leftMid, qMax(0,i-17)/6.0f);
        rightColors[i] = blendColors(rightLow, rightMid, qMax(0,i-17)/6.0f);
    }
    for(int i=23; i<32; i++)
    {
        leftColors[i] = blendColors(leftMid, leftHigh, qMax(0,i-28)/3.0f);
        rightColors[i] = blendColors(rightMid, rightHigh, qMax(0,i-28)/3.0f);
    }
}

void PeakMeter::timerTimeout()
{
    static float prevPeaks[2];
    static float prevMax[2];

    for(int i=0; i<2; i++)
    {
        max[i] = qMax(0.0f, max[i]-(float)qPow(speed[i],12.0f));

        speed[i] += 0.008f;

        if(max[i] < peaks[i])
        {
            max[i] = qCeil(peaks[i]*32)/32.0f;
            speed[i] = 0.0f;
        }

        if(max[i] <= 1/32.0f)
        {
            max[i] = -1.0f;
        }

        peaks[i] = qMax(0.0f, peaks[i]-0.04f);
    }

    if((prevPeaks[0] != peaks[0]) ||
            (prevPeaks[1] != peaks[1]) ||
            (prevMax[0] != max[0]) ||
            (prevMax[1] != max[1]))
    {
        this->update();
    }

    prevPeaks[0] = peaks[0];
    prevPeaks[1] = peaks[1];
    prevMax[0] = max[0];
    prevMax[1] = max[1];
}
