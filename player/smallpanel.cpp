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
    layout->setContentsMargins(0, 0, 0, 0);

    nameLbl = new QLabel;
    nameLbl->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    layout->addWidget(nameLbl, 1);
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
    this->playerStateChanged(false);
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

void SmallPanel::playerStateChanged(bool isPlaying)
{
    if(!isPlaying)
    {
        nameLbl->setText(player->getPluginName());
    }
}

void SmallPanel::playerPositionChanged(double pos, double duration, bool autoplay)
{
    //QString text = autoplay ? tr("Autoplay\n") : "\n";
    //text += QString::number(number) + "\n";

    QString text;

    if(pos >= 0 && duration > 0)
    {
        QString posStr = QTime(0, 0).addSecs(pos).toString("\nmm:ss");
        QString durationStr = QTime(0, 0).addSecs(duration).toString("/mm:ss");
        text = "\n" + player->getPluginName() + posStr + durationStr;
    }
    else
    {
        text = player->getPluginName();
    }

    nameLbl->setText(text);
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

void SmallPanel::addThumbnail(ThumbnailWidget *thumb)
{
    thumb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(thumb, 1);
}

void SmallPanel::removeThumbnail(ThumbnailWidget *thumb)
{
    layout->removeWidget(thumb);
}
