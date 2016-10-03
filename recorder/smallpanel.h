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
#include <QTime>

class Recorder;

class SmallPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SmallPanel(Recorder *recorder);

    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void retranslate();
    void changeEvent(QEvent *e);

    void addPeakMeter(PeakMeter *peakMeter);
    void removePeakMeter(PeakMeter *peakMeter);

private:
    void recalcSizes(const QSize &size);

    Recorder *recorder;
    QVBoxLayout *layout;
    QLabel *nameLbl;
    QLabel *infoLbl;

public slots:
    void recorderStateChanged();
    void recorderPositionChanged(QTime position);
};

#endif // SMALLPANEL_H
