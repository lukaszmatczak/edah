#include "waveform.h"

#include <QPainter>

Waveform::Waveform(QWidget *parent) : QSlider(parent)
{

}

void Waveform::setWaveform(const QByteArray &form)
{
    this->form = form;
    this->update();
}

void Waveform::paintEvent(QPaintEvent *e)
{
    QSlider::paintEvent(e);

    QPainter p(this);

    p.setRenderHints(QPainter::Antialiasing);
    p.scale(this->width()/(1024.0+(1024.0/this->width()*3)), this->height()/(256.0+256.0/this->height()*3));

    for(int i=0; i<form.size()/2; i++)
    {
        if(i <= this->value()*1024/this->maximum())
        {
            p.setPen(QColor(127, 192, 255));
        }
        else
        {
            p.setPen(QColor(127, 127, 127));
        }

        p.drawLine((1024.0/this->width())+i,
                   (256.0/this->height())+255-(quint8)form.at(i*2)-1,
                   (1024.0/this->width())+i,
                   (256.0/this->height())+255-(quint8)form.at(i*2+1));
    }
}
