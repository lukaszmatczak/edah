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
#include "stream.h"

#include <QApplication>
#include <QResizeEvent>

#include <QDebug>

BigPanel::BigPanel(Stream *stream) : QWidget(0), stream(stream)
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

    startBtn = new MyPushButton("", this);
    startBtn->setIcon(QIcon(":/stream-img/record.svg"));
    startBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(startBtn, &MyPushButton::clicked, this, &BigPanel::startBtn_clicked);
    layout->addWidget(startBtn, 1, 1);

    statusTxt = new QTextEdit(this);
    statusTxt->setObjectName("statusTxt");
    statusTxt->setReadOnly(true);
    statusTxt->setTextInteractionFlags(Qt::NoTextInteraction);
    statusTxt->viewport()->setCursor(Qt::ArrowCursor);
    flick.activateOn(statusTxt);
    layout->addWidget(statusTxt, 2, 0, 1, 4);

    this->retranslate();
}

BigPanel::~BigPanel()
{

}

void BigPanel::retranslate()
{

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
    this->setStyleSheet(QString("#timeLbl {"
                                "   font-size: %1px;"
                                "}"
                                "#statusTxt {"
                                "   background-color: rgb(36,36,36);"
                                "   border-color:  rgb(0,0,0);"
                                "   border-bottom-color: rgb(70, 70, 70);"
                                "   border-right-color:  rgb(70, 70, 70);"
                                "   border-width : 2 1 1 2px;"
                                "   border-style: solid;"
                                "   font-size: %1px;"
                                "}"
                                "QScrollBar:vertical {"
                                "  border: 0px;"
                                "  background: rgb(36,36,36);"
                                "  width: 8px;"
                                "}"
                                "QScrollBar::handle:vertical {"
                                "  background: white;"
                                "  min-height: 10px;"
                                "  border-radius: 4px;"
                                "}"
                                "QScrollBar::handle:hover:vertical {"
                                "  background: rgb(200,200,200);"
                                " }"
                                "QScrollBar::add-line:vertical {"
                                "  background: none;"
                                "}"
                                "QScrollBar::sub-line:vertical {"
                                "  background: none;"
                                "}"
                                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                                "  background: none;"
                                "}")
                        .arg(qMax(1, size.height()/24)));

    QSize iconSize = QSize(size.height()/5, size.height()/5);
    startBtn->setIconSize(iconSize);
}

void BigPanel::startBtn_clicked()
{
    if(stream->isActive())
    {
        startBtn->setIcon(QIcon(":/stream-img/record.svg"));
        qApp->processEvents();
        emit stop();
    }
    else
    {
        startBtn->setIcon(QIcon(":/stream-img/stop.svg"));
        qApp->processEvents();
        emit start();
    }

    this->update();
}

void BigPanel::streamStateChanged()
{
    if(stream->isActive())
    {
        startBtn->setIcon(QIcon(":/stream-img/stop.svg"));
    }
    else
    {
        startBtn->setIcon(QIcon(":/stream-img/record.svg"));
        timeLbl->setText("-:--:--");
    }
}

void BigPanel::streamPositionChanged(QTime position)
{
    if(position.isValid())
        timeLbl->setText(position.toString("H:mm:ss"));
}

void BigPanel::addStatus(const QString &text)
{
    QString date = QDateTime::currentDateTime().toString("'['yy-MM-dd HH:mm:ss'] '");

    if(!statusTxt->toPlainText().isEmpty())
        statusTxt->insertPlainText("\n");

    statusTxt->insertPlainText(date + text);

    statusTxt->ensureCursorVisible();
}
