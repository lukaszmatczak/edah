#include "peakmeter.h"

#include "logger.h"

#include <QPainter>
#include <QtMath>
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

bool PeakMeter::hasHeightForWidth() const
{
    //return true;
    return false;
}

int PeakMeter::heightForWidth(int width) const
{
    return width*2;
}

void PeakMeter::setPeak(float left, float right)
{
    peaks[0] = qMax(peaks[0], left);
    peaks[1] = qMax(peaks[1], right);

    this->update();
}

void PeakMeter::setChannels(int count)
{
    this->channels = count;
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

    if(this->height()/this->width() < 1580.0/450.0)
    {
        int destWidth = this->height()*450.0/1580.0;
        margin = (this->width()-destWidth)/2;
    }

    p.setRenderHints(QPainter::Antialiasing);
    qreal hScale = (this->width()-margin*2)/450.0;
    p.scale(hScale, this->height()/1580.0);

    QRadialGradient gradient(0.5, 0.5, 0.5);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    for(int channel=0; channel<this->channels; channel++)
    {
        for(int i=0; i<32; i++)
        {
            QColor color = QColor::fromRgb(colors[i]);

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
        colors[i] = blendColors(low, mid, qMax(0,i-17)/6.0f);
    }
    for(int i=23; i<32; i++)
    {
        colors[i] = blendColors(mid, high, qMax(0,i-28)/3.0f);
    }
}

void PeakMeter::timerTimeout()
{
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
}
