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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "winframe.h"
#include <libedah/iplugin.h>
#include <libedah/updater.h>

#include <QMainWindow>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QTimer>
#include <QTranslator>
#include <QPushButton>
#include <QSettings>
#include <QPluginLoader>
#include <QThread>
#include <QSystemTrayIcon>

struct Plugin
{
    QString id;
    QPluginLoader *loader;
    IPlugin *plugin;
    QWidget *panel;
    QWidget *container;
    //QWidget *widget;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void reloadPlugins();
    void showWindow();

protected:
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *e);
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    void refreshPluginContainerText();

    bool loadPlugin(const QString &id, Plugin *plugin);
    void unloadPlugin(Plugin *plugin);

    QTranslator translator;

#ifdef Q_OS_WIN
    QSettings *globalSettings;
#endif
    bool experimental;

    QWidget *centralWidget;
    WinFrame *winFrame;
    QFrame *container;
    QLabel *pluginContainer;
    QHBoxLayout *pluginLayout;

    QSystemTrayIcon *trayIcon;

    //QLabel *titleLbl;
    //QToolButton *minimizeBtn;
    //QToolButton *closeBtn;

    QVector<Plugin> plugins;

#ifdef Q_OS_WIN
    QThread updaterThread;
    Updater *updater;
#endif
    bool updateAvailable;
    UpdateInfoArray updateInfo;

public slots:
    void newProcess(const QString &message);

private slots:
    void recalcSizes(QSize size);
    void settingsChanged();
    void onMaximizeBtnClicked();
    void showMenu(QSystemTrayIcon::ActivationReason reason);
    void showAbout();
    void showUpdateDialog();
    void showSettings();
    void showUpdate(UpdateInfoArray info);

signals:
    void checkForUpdates();
    void loadProgressChanged(int curr, int max);
};

#endif // MAINWINDOW_H
