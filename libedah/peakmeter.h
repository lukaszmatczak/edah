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

#ifndef PEAKMETER_H
#define PEAKMETER_H

#include "libedah.h"

#include <QWidget>
#include <QTimer>

class LIBEDAHSHARED_EXPORT PeakMeter : public QWidget
{
    Q_OBJECT
public:
    explicit PeakMeter(QWidget *parent = 0);
    virtual ~PeakMeter();

    void setPeak(float left, float right);
    void setColors(QRgb low, QRgb mid, QRgb high);
    void setColors(QRgb leftLow, QRgb leftMid, QRgb leftHigh,
                   QRgb rightLow, QRgb rightMid, QRgb rightHigh);
    void setChannels(int count);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QRgb blendColors(QRgb color1, QRgb color2, float r);

    float peaks[2];
    float max[2];
    float speed[2];

    int channels;

    QRgb leftColors[32];
    QRgb rightColors[32];

    QTimer timer;

public slots:
    //void setPeakLevel(QAudioBuffer buffer);

private slots:
    void timerTimeout();
};

#endif // PEAKMETER_H
