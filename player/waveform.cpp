#include "waveform.h"

#include <QPainter>

Waveform::Waveform(QWidget *parent) : QSlider(parent), form(nullptr)
{

}

void Waveform::setWaveform(QByteArray *form)
{
    this->form = form;
    this->update();
}

void Waveform::paintEvent(QPaintEvent *e)
{
    //QSlider::paintEvent(e);

    if(!form)
    {
        return;
    }

    QPainter p(this);

    p.setRenderHints(QPainter::Antialiasing);
    p.scale(this->width()/1024.0, this->height()/256.0);

    p.setBrush(QBrush(QColor(0, 80, 255)));
    p.setPen(Qt::NoPen);
    p.drawRect(0, 0, this->value()*1024/this->maximum(), 256);

    for(int i=0; i<form->size()/2; i++)
    {
        if(i <= this->value()*1024/this->maximum())
        {
            p.setPen(QColor(127, 192, 255));
        }
        else
        {
            p.setPen(QColor(127, 127, 127));
        }

        p.drawLine(i, 255-(quint8)form->at(i*2), i, 255-(quint8)form->at(i*2+1));
    }
}
