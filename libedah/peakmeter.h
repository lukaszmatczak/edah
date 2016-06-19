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

    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;

    void setPeakStereo(float left, float right);
    void setColors(QRgb low, QRgb mid, QRgb high);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QRgb blendColors(QRgb color1, QRgb color2, float r);

    float peaks[2];
    float max[2];
    float speed[2];

    QRgb colors[32];

    QTimer timer;

public slots:
    //void setPeakLevel(QAudioBuffer buffer);

private slots:
    void timerTimeout();
};

#endif // PEAKMETER_H
