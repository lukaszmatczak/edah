#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QSlider>

class Waveform : public QSlider
{
public:
    Waveform(QWidget *parent);

    void setWaveform(QByteArray *form);

protected:
    void paintEvent(QPaintEvent *e);

private:
    QByteArray *form;
};

#endif // WAVEFORM_H
