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
#include "keypad.h"

#include <libedah/mypushbutton.h>
#include <libedah/peakmeter.h>
#include <libedah/thumbnailwidget.h>
#include <libedah/flickcharm.h>

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QProxyStyle>
#include <QTableView>

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


class BigPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BigPanel(Player *player);
    virtual ~BigPanel();

    void addPeakMeter(PeakMeter *peakMeter);
    void removePeakMeter(PeakMeter *peakMeter);
    void addThumbnail(ThumbnailWidget *thumb);
    void removeThumbnail(ThumbnailWidget *thumb);

    void showKeyboard(int number);

    void playerPositionChanged(bool paused, double pos, double duration);
    void setCurrentPlaylistEntry(int n);

    void retranslate();


    QTableView *playlistView;
    Waveform *posBar;

protected:
    bool event(QEvent *event);
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void changeEvent(QEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);

private:
    void recalcSizes(const QSize &size);
    void updateTitle(int number);
    void setNonstop(bool isSet);


    Player *player;

    QGridLayout *layout;

    FlickCharm flick;

    QLabel *titleLbl;
    MyPushButton *rndBtn;
    MyPushButton *playBtn;
    MyPushButton *stopBtn;
    MyPushButton *keyboardBtn;
    QFrame *songInfoFrm;
    QWidget *playlistBtnArea;
    MyPushButton *addFileBtn;
    MyPushButton *addWindowBtn;
    MyPushButton *removeFileBtn;
    MyPushButton *UpBtn;
    MyPushButton *DownBtn;

    QPushButton *nonstopIcon;
    QLabel *nonstopLbl;
    QLabel *posLbl;

    SliderStyle sliderStyle;
    double currDuration;
    bool nonstop;

public slots:
    //void playerStateChanged(bool isPlaying);

private slots:
    void btnRnd_clicked();
    void playBtn_clicked();
    void stopBtn_clicked();
    void posBar_valueChanged(int value);
    void posBar_released();
    void keyboardBtn_clicked();

    void playlist_dblClicked(const QModelIndex &index);
    void addFileBtn_clicked();
    void removeFileBtn_clicked();
    void UpBtn_clicked();
    void DownBtn_clicked();

signals:
    void play(int entry);
    void playSong(int number, bool autoplay);
    void stop();
    void seek(int ms);
};

#endif // BIGPANEL_H
