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

#include "winframe.h"
#include "iplugin.h"

#include <QMainWindow>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QTimer>
#include <QTranslator>

#include <QPluginLoader>

struct Plugin
{
    QString id;
    QPluginLoader *loader;
    IPlugin *plugin;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void showEvent(QShowEvent *e);
    void changeEvent(QEvent *e);

private:
    bool loadPlugin(const QString &id, Plugin *plugin);
    void loadPlugins();
    void refreshPlugins();
    void settingsChanged();

    void addShadowEffect(QWidget *widget, QColor color);
    void createTitleBar(QWidget *parent);
    void createBottomBar(QWidget *parent);
    void updateStyle(QWidget *widget);

    QTranslator *translator;

    QWidget *centralWidget;
    WinFrame *winFrame;
    QFrame *container;
    QWidget *pluginContainer;

    QFrame *titleBar;
    QToolButton *menuBtn;
    QLabel *titleLbl;
    QToolButton *minimizeBtn;
    QToolButton *maximizeBtn;
    QToolButton *closeBtn;

    QFrame *bottomBar;
    QToolButton *closeBtn_bottom;
    QToolButton *minimizeBtn_bottom;
    QToolButton *menuBtn_bottom;
    QLabel *clockLbl;

    QPoint movePos;
    QTimer timer;

    QVector<Plugin> plugins;
    QVector<QWidget*> lines;

public slots:
    void newProcess(const QString &message);

private slots:
    void timerSlot();

    void onFocusChanged(QWidget *old, QWidget *now);

    void recalcSizes(QSize size);

    void onMaximizeBtnClicked();
    void showMenu();

    void showAbout();
    void showSettings();

};

#endif // MAINWINDOW_H
