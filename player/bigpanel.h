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

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>

class Player;

class BigPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BigPanel(Player *player);
    void setPeak(float left, float right);

protected:
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);

private:
    void recalcSizes(const QSize &size);
    void updateTitle(int number);

    Player *player;

    QGridLayout *layout;
    QVector<MyPushButton*> numberBtns;
    QLabel *numberLbl;
    QWidget *titleLine;
    QLabel *titleLbl;
    MyPushButton *playBtn;

    PeakMeter *peakMeter;

private slots:
    void numberBtn_clicked();
    void btnBack_clicked();
    void playBtn_clicked();

signals:
    void play(int number);
    void stop();
};

#endif // BIGPANEL_H
