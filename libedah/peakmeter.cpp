#include "peakmeter.h"

#include "logger.h"

#include <QPainter>
#include <QtMath>
#include <QDebug>

PeakMeter::PeakMeter(QWidget *parent) : QWidget(parent), stopped(false)
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
    return true;
}

int PeakMeter::heightForWidth(int width) const
{
    return width*2;
}

void PeakMeter::stop()
{
    stopped = true;
}

void PeakMeter::setPeakLevel(QAudioBuffer buffer)
{
    float peak[2] = {2.0f, 2.0f};

    QAudioFormat format = buffer.format();

    for(int i=0; i<qMin(2, format.channelCount()); i++)
    {
        if(format.sampleType() == QAudioFormat::SignedInt)
        {
            if(format.sampleSize() == 8)       peak[i] = this->calcMax<qint8>(buffer, i);
            else if(format.sampleSize() == 16) peak[i] = this->calcMax<qint16>(buffer, i);
            else if(format.sampleSize() == 32) peak[i] = this->calcMax<qint32>(buffer, i);
        }
        else if(format.sampleType() == QAudioFormat::UnSignedInt)
        {
            if(format.sampleSize() == 8)       peak[i] = this->calcMax<quint8>(buffer, i);
            else if(format.sampleSize() == 16) peak[i] = this->calcMax<quint16>(buffer, i);
            else if(format.sampleSize() == 32) peak[i] = this->calcMax<quint32>(buffer, i);
        }
        else if(format.sampleType() == QAudioFormat::Float)
        {
            peak[i] = this->calcMax<float>(buffer, i);
        }
    }

    if(peak[0] == 2.0f)
    {
        LOG(QString("Unsupported format (sampleType=%1, sampleSize=%2")
            .arg(format.sampleType())
            .arg(format.sampleSize()));
    }

    if(format.channelCount() == 1)
    {
        peak[1] = peak[0];
    }

    this->setPeakStereo(peak[0], peak[1]);
}

void PeakMeter::setPeakStereo(float left, float right)
{
    stopped = false;

    peaks[0] = qMax(peaks[0], left);
    peaks[1] = qMax(peaks[1], right);

    this->update();
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

    p.setRenderHints(QPainter::Antialiasing);
    p.scale(this->width()/1000.0, this->height()/1580.0);

    QRadialGradient gradient(0.5, 0.5, 0.5);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    for(int channel=0; channel<2; channel++)
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
            p.drawRoundedRect((channel*250)+275, 1580-(i*50), 200, 30, 10, 100);
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

    if(stopped)
    {
        this->update();
        if((max[0] == 0.0f) && (max[1] == 0.0f))
        {
            stopped = false;
        }
    }
}
