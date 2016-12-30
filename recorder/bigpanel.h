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
#include <QLineEdit>
#include <QTime>

class Recorder;

class BigPanel : public QWidget
{
    Q_OBJECT
public:
    explicit BigPanel(Recorder *recorder);
    ~BigPanel();

    void retranslate();
    void addPeakMeter(PeakMeter *peakMeter);
    void removePeakMeter(PeakMeter *peakMeter);
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void recalcSizes(QSize size);

    QLineEdit *nameEdit;

protected:
    bool event(QEvent *event);

private:
    Recorder *recorder;

    QGridLayout *layout;
    MyPushButton *recBtn;
    QLabel *timeLbl;
    QLabel *nameLbl;

public slots:
    void recorderStateChanged();
    void recorderPositionChanged(QTime position);

private slots:
    void recBtn_clicked();

signals:
    void record();
    void stop(QString filename);
};

#endif // BIGPANEL_H
