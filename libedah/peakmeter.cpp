#include "peakmeter.h"

#include <QPainter>
#include <QtMath>
#include <QDebug>

PeakMeter::PeakMeter(QWidget *parent) : QWidget(parent)
{
    colorHigh = qRgb(255, 0, 0);
    colorMid = qRgb(255, 255, 0);
    colorLow = qRgb(0, 0, 255);

    peaks[0] = 0.0f;
    peaks[1] = 0.0f;

    max[0] = 0.0f;
    max[1] = 0.0f;

    speed[0] = 0.0f;
    speed[1] = 0.0f;

    timer.setInterval(35);
    connect(&timer, &QTimer::timeout, this, &PeakMeter::timerTimeout);
    timer.start();
}

PeakMeter::~PeakMeter()
{

}

bool PeakMeter::hasHeightForWidth() const
{
    return true;
}

int PeakMeter::heightForWidth(int width) const
{
    return width*2;
}

void PeakMeter::setPeakLevel(float left, float right)
{
    peaks[0] = qMax(peaks[0], left);
    peaks[1] = qMax(peaks[1], right);

    this->update();
}

QRgb blendColors(QRgb color1, QRgb color2, float r)
{
     return qRgb(qRed(color1)* (1-r) + qRed(color2)*r,
    qGreen(color1)*(1-r) + qGreen(color2)*r,
    qBlue(color1)*(1-r) + qBlue(color2)*r);
}

void PeakMeter::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter p(this);

    p.setRenderHints(QPainter::Antialiasing);
    p.scale(this->width()/1000.0, this->height()/1580.0);

    QRadialGradient gradient(0.5, 0.5, 0.5);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    QColor color;

    for(int channel=0; channel<2; channel++)
    {
        for(int i=0; i<32; i++)
        {
            if(i < 23) color = blendColors(colorLow, colorMid, qMax(0,i-17)/7.0f);
            else if(i < 31) color = blendColors(colorMid, colorHigh, qMax(0,i-28)/4.0f);
            else color = colorHigh;

            if(i/32.0 > peaks[channel] && (qAbs(max[channel]*31-i) > 0.5f))
            {
                color = color.lighter(8);
            }

            gradient.setColorAt(0, color.lighter(120));
            gradient.setColorAt(1, color);
            p.setBrush(QBrush(gradient));
            p.setPen(color);
            p.drawRoundedRect((channel*250)+275, 1580-(i*50), 200, 30, 10, 100);
        }
    }
}

void PeakMeter::setColors(QRgb low, QRgb mid, QRgb high)
{
    colorLow = low;
    colorMid = mid;
    colorHigh = high;
}

void PeakMeter::timerTimeout()
{
    for(int i=0; i<2; i++)
    {
        peaks[i] = qMax(0.0f, peaks[i]-0.04f);

        max[i] = qMax<float>(0.0f, max[i]-qPow(speed[i],12.0f));
        speed[i] += 0.008f;

        if(max[i] < peaks[i])
        {
            max[i] = qCeil(peaks[i]*32+0.5f)/32.0f;
            speed[i] = 0.0f;
        }
    }
}
