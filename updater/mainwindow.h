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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTime>
#include <QThread>
#include <QSettings>
#include <QTranslator>

#include <libedah/updater.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *e);

private:
    QSettings *settings;
    Updater *updater;
    QThread updaterThread;

    QTranslator translator;

    QWidget *central;
    QVBoxLayout *layout;
    QLabel *stepLbl;
    QLabel *progressLbl;
    QProgressBar *progressBar;

    bool canClose;
    bool isDialogVisible;
    QTime lastUpdateTime;
    int lastProgress;

private slots:
    void progress(int step, int curr, int max);
    void done();

signals:
    void doUpdate();
};

#endif // MAINWINDOW_H
