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

#ifndef BIGPANEL_H
#define BIGPANEL_H

#include <libedah/mypushbutton.h>
#include <libedah/peakmeter.h>
#include <libedah/flickcharm.h>

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTime>
#include <QTextEdit>
#include <QTimer>

class Stream;

class BigPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BigPanel(Stream *stream);
    ~BigPanel();

    void retranslate();
    void addPeakMeter(PeakMeter *peakMeter);
    void removePeakMeter(PeakMeter *peakMeter);
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void recalcSizes(QSize size);

    void addStatus(const QString &text);

private:
    Stream *stream;

    FlickCharm flick;

    QGridLayout *layout;
    MyPushButton *startBtn;
    QLabel *timeLbl;
    QTextEdit *statusTxt;

    QTimer singleTimer;

public slots:
    void streamStateChanged();
    void streamPositionChanged(QTime position);

private slots:
    void startBtn_clicked();

signals:
    void start();
    void stop();
};

#endif // BIGPANEL_H
