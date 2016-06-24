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
#include "player.h"

#include <QResizeEvent>
#include <QTime>

SmallPanel::SmallPanel(Player *player) : player(player)
{
    layout = new QVBoxLayout;
    this->setLayout(layout);

    QLabel *nameLbl = new QLabel(player->getPluginName());
    nameLbl->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(nameLbl, 1);

    infoLbl = new QLabel("\n-\n-:--/-:--");
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

void SmallPanel::playerStateChanged(bool isPlaying)
{
    if(!isPlaying)
    {
        infoLbl->setText("\n-\n-:--/-:--");
    }
}

void SmallPanel::playerPositionChanged(int number, double pos, double duration, bool autoplay)
{
    QString text = autoplay ? tr("Autoplay\n") : "\n";
    text += QString::number(number) + "\n";

    QString posStr = "-:--";
    QString durationStr = "/-:--";
    if(pos > -1) posStr = QTime(0, 0).addSecs(pos).toString("m:ss");
    if(duration > -1) durationStr = QTime(0, 0).addSecs(duration).toString("/m:ss");
    text += posStr + durationStr;

    infoLbl->setText(text);
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
