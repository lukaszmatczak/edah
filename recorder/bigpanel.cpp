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

#include "bigpanel.h"
#include "recorder.h"

#include <QApplication>
#include <QResizeEvent>

#include <QDebug>

BigPanel::BigPanel(Recorder *recorder) : QWidget(0), recorder(recorder)
{
    layout = new QGridLayout(this);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 2);
    layout->setRowStretch(2, 1);
    layout->setColumnStretch(0, 3);
    layout->setColumnStretch(1, 4);
    layout->setColumnStretch(2, 2);
    layout->setColumnStretch(3, 3);
    this->setLayout(layout);

    timeLbl = new QLabel("-:--:--", this);
    timeLbl->setObjectName("timeLbl");
    timeLbl->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    layout->addWidget(timeLbl, 0, 0, 1, 4);

    recBtn = new MyPushButton("", this);
    recBtn->setIcon(QIcon(":/recorder-img/record.svg"));
    recBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(recBtn, &MyPushButton::clicked, this, &BigPanel::recBtn_clicked);
    layout->addWidget(recBtn, 1, 1);

    QVBoxLayout *nameLayout = new QVBoxLayout;
    nameLayout->addStretch();
    nameLayout->setSpacing(0);

    nameEdit = new QLineEdit(this);
    nameEdit->setObjectName("nameEdit");
    nameEdit->setAlignment(Qt::AlignCenter);
    nameLayout->addWidget(nameEdit);

    nameLbl = new QLabel(this);
    nameLbl->setObjectName("nameLbl");
    nameLbl->setAlignment(Qt::AlignCenter);
    nameLayout->addWidget(nameLbl);

    layout->addLayout(nameLayout, 2, 0, 1, 4);

    this->retranslate();
}

BigPanel::~BigPanel()
{

}

bool BigPanel::event(QEvent *event)
{
    if(event->type() == QEvent::FocusOut)
    {
        nameEdit->releaseKeyboard();
    }
    else if(event->type() == QEvent::FocusIn)
    {
        nameEdit->grabKeyboard();
    }

    return QWidget::event(event);
}

void BigPanel::retranslate()
{
    nameLbl->setText(tr("Name of file"));
}

void BigPanel::addPeakMeter(PeakMeter *peakMeter)
{
    peakMeter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(peakMeter, 1, 2);
}

void BigPanel::removePeakMeter(PeakMeter *peakMeter)
{
    layout->removeWidget(peakMeter);
}

void BigPanel::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    recalcSizes(this->size());
}

void BigPanel::resizeEvent(QResizeEvent *e)
{
    this->recalcSizes(e->size());
}

void BigPanel::recalcSizes(QSize size)
{
    this->setStyleSheet(QString("#nameEdit {"
                                "   background-color: rgb(20,20,20);"
                                "   border-bottom-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0.15 rgba(255, 255, 255, 0), stop:0.5 rgba(255,255,255,255), stop:0.85 rgba(255,255,255,0));"
                                "   border-width: 0px;"
                                "   border-bottom-width: 2px;"
                                "   border-style: solid;"
                                "}"
                                ""
                                "#nameLbl {"
                                "   font-size: %1px;"
                                "}"
                                "#timeLbl {"
                                "   font-size: %2px;"
                                "}")
                        .arg(qMax(1, size.height()/32))
                        .arg(qMax(1, size.height()/24)));

    QSize iconSize = QSize(size.height()/5, size.height()/5);
    recBtn->setIconSize(iconSize);
}

void BigPanel::recBtn_clicked()
{
    if(recorder->isRecording())
    {
        recBtn->setIcon(QIcon(":/recorder-img/record.svg"));
        qApp->processEvents();
        emit stop(nameEdit->text());
        nameEdit->clear();
    }
    else
    {
        recBtn->setIcon(QIcon(":/recorder-img/stop.svg"));
        qApp->processEvents();
        emit record();
    }

    this->update();
}

void BigPanel::recorderStateChanged()
{
    if(recorder->isRecording())
    {
        recBtn->setIcon(QIcon(":/recorder-img/stop.svg"));
    }
    else
    {
        recBtn->setIcon(QIcon(":/recorder-img/record.svg"));
        timeLbl->setText("-:--:--");
    }
}

void BigPanel::recorderPositionChanged(QTime position)
{
    if(position.isValid())
        timeLbl->setText(position.toString("H:mm:ss"));
}
