/*
    Edah
    Copyright (C) 2016  Lukasz Matczak

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

#include "smallpanel.h"
#include "stream.h"

#include <QResizeEvent>

SmallPanel::SmallPanel(Stream *stream) : stream(stream)
{
    layout = new QVBoxLayout;
    this->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    nameLbl = new QLabel;
    nameLbl->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(nameLbl, 1);

    infoLbl = new QLabel("-:--:--");
    infoLbl->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
    layout->addWidget(infoLbl, 1);
}

void SmallPanel::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    this->recalcSizes(this->size());
}

void SmallPanel::resizeEvent(QResizeEvent *e)
{
    this->recalcSizes(e->size());
}

void SmallPanel::recalcSizes(const QSize &size)
{
    this->setStyleSheet(QString("QLabel {"
                                "   font-size: %1px;"
                                "}")
                        .arg(qMax(1, size.height()/24)));
}

void SmallPanel::retranslate()
{
    nameLbl->setText(stream->getPluginName());
}

void SmallPanel::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        this->retranslate();
    }
    else
    {
        QWidget::changeEvent(e);
    }
}

void SmallPanel::streamStateChanged()
{
    if(!stream->isActive())
    {
        nameLbl->setStyleSheet("");
        infoLbl->setText("-:--:--");
    }
    else
    {
        nameLbl->setStyleSheet("color: red;");
    }
}

void SmallPanel::streamPositionChanged(QTime position)
{
    if(position.isValid())
        infoLbl->setText(position.toString("H:mm:ss"));
}

void SmallPanel::addPeakMeter(PeakMeter *peakMeter)
{
    peakMeter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->insertWidget(1, peakMeter, 2);
}

void SmallPanel::removePeakMeter(PeakMeter *peakMeter)
{
    layout->removeWidget(peakMeter);
}
