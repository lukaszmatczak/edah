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

#include "waveform.h"

#include <libedah/mypushbutton.h>
#include <libedah/peakmeter.h>

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QProxyStyle>

#include <random>

class Player;
struct Song;

class SliderStyle : public QProxyStyle
{
public:
    virtual int styleHint(StyleHint hint, const QStyleOption *option=0, const QWidget *widget=0, QStyleHintReturn *returnData=0) const
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
        {
            return Qt::LeftButton;
        }
        else
        {
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }
};

class ShufflePlaylist : public QObject
{
    Q_OBJECT
public:
    ShufflePlaylist(QMap<int, Song> *songs);
    virtual ~ShufflePlaylist();
    int getNext();

private:
    void generateNewPlaylist();
    void shuffle(QVector<int>& vec);

    QMap<int, Song> *songs;
    QVector<int> playlist;

    std::mt19937 *mtEngine;
    int currPos;
};

class BigPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BigPanel(Player *player);
    virtual ~BigPanel();

    void addPeakMeter(PeakMeter *peakMeter);

    void playerPositionChanged(double pos, double duration);

protected:
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void keyReleaseEvent(QKeyEvent *e);

private:
    void recalcSizes(const QSize &size);
    void updateTitle(int number);
    void addDigit(int digit);
    void setNonstop(bool isSet);

    Player *player;
    ShufflePlaylist *rndPlaylist;

    QGridLayout *layout;
    QVector<MyPushButton*> numberBtns;
    QLabel *numberLbl;
    QWidget *titleLine;
    QLabel *titleLbl;
    MyPushButton *rndBtn;
    MyPushButton *btnBack;
    MyPushButton *playBtn;
    QPushButton *nonstopIcon;
    QLabel *nonstopLbl;
    QLabel *posLbl;
    Waveform *posBar;
    SliderStyle sliderStyle;
    double currDuration;
    bool nonstop;

public slots:
    void playerStateChanged(bool isPlaying);

private slots:
    void btnRnd_clicked();
    void numberBtn_clicked();
    void btnBack_clicked();
    void playBtn_clicked();
    void posBar_valueChanged(int value);
    void posBar_released();

signals:
    void play(int number);
    void stop();
    void seek(int ms);
};

#endif // BIGPANEL_H
