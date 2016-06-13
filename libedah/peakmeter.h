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

    void setPeakLevel(float left, float right);
    void setColors(QRgb low, QRgb mid, QRgb high);

protected:
    void paintEvent(QPaintEvent *e);

private:
    float peaks[2];
    float max[2];
    float speed[2];

    QRgb colorLow, colorMid, colorHigh;

    QTimer timer;

private slots:
    void timerTimeout();
};

#endif // PEAKMETER_H
