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

#ifndef SMALLPANEL_H
#define SMALLPANEL_H

#include <libedah/peakmeter.h>

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>

class Player;

class SmallPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SmallPanel(Player *player);
    void playerPositionChanged(int number, double pos, double duration, bool autoplay);

    void addPeakMeter(PeakMeter *peakMeter);
    void removePeakMeter(PeakMeter *peakMeter);

    void retranslate();

protected:
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void changeEvent(QEvent *e);

public slots:
    void playerStateChanged(bool isPlaying);

private:
    void recalcSizes(const QSize &size);

    Player *player;

    QVBoxLayout *layout;
    QLabel *nameLbl;
    QLabel *infoLbl;
};

#endif // SMALLPANEL_H
