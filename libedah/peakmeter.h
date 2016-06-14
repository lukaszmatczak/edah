#ifndef PEAKMETER_H
#define PEAKMETER_H

#include "libedah.h"

#include <QWidget>
#include <QTimer>

#include <QAudioBuffer>

class LIBEDAHSHARED_EXPORT PeakMeter : public QWidget
{
    Q_OBJECT
public:
    explicit PeakMeter(QWidget *parent = 0);
    virtual ~PeakMeter();

    bool hasHeightForWidth() const;
    int heightForWidth(int width) const;

    void stop();

    void setPeakStereo(float left, float right);
    void setColors(QRgb low, QRgb mid, QRgb high);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QRgb blendColors(QRgb color1, QRgb color2, float r);

    template <typename T>
    float calcMax(const QAudioBuffer &buffer, int channel)
    {
        const T *data = buffer.data<T>();
        T ret = 0;

        const int count = buffer.frameCount();
        const int channelCount = buffer.format().channelCount();

        for(int i=0; i<count-channelCount; i+=channelCount)
        {
            ret = qMax(ret, data[i+channel]);
        }

        if(std::is_same<T, float>::value)
        {
            return ret;
        }

        return ret / (float)std::numeric_limits<T>::max();
    }

    float peaks[2];
    float max[2];
    float speed[2];

    QRgb colors[32];

    QTimer timer;
    bool stopped;

public slots:
    void setPeakLevel(QAudioBuffer buffer);

private slots:
    void timerTimeout();
};

#endif // PEAKMETER_H
