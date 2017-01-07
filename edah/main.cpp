/*
    Edah
    Copyright (C) 2016-2017  Lukasz Matczak

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

#include "mainwindow.h"
#include "splashscreen.h"
#include "qtsingleapplication.h"

#include <QThread>

int main(int argc, char *argv[])
{
    QtSingleApplication a(argc, argv);

    if(a.isRunning())
    {
        if(argc == 1) return 0;
        QThread::msleep(500);
        return !a.sendMessage(QString::fromLocal8Bit(argv[1]));
    }

    QPixmap pix(":/img/splash.png");
    SplashScreen splash(pix);
    splash.show();
    a.processEvents();

    MainWindow w;
    QObject::connect(&a, &QtSingleApplication::messageReceived, &w, &MainWindow::newProcess);
    QObject::connect(&w, &MainWindow::loadProgressChanged, &splash, &SplashScreen::setProgress);

    QStringList args = a.arguments();
    bool rescue = false;

    for(int i=0; i<args.size(); i++)
    {
        if(args[i] == "--rescue")
        {
            rescue = true;
            break;
        }
    }

    if(!rescue)
        w.reloadPlugins();

    w.show();

    splash.finish(&w);

    return a.exec();
}
