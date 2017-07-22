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
#include "aboutdialog.h"
#include "updatedialog.h"
#include "settings.h"

#include <libedah/logger.h>
#include <libedah/utils.h>

#include <QResizeEvent>
#include <QFontDatabase>
#include <QTime>
#include <QApplication>
#include <QDir>
#include <QGraphicsDropShadowEffect>
#include <QTimeLine>
#include <QMenu>
#include <QThreadPool>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), updateAvailable(false)
{
    QCoreApplication::setOrganizationName("Lukasz Matczak");
    QCoreApplication::setApplicationName("Edah");

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount()/2);

    utils = new Utils(this);
    logger = new Logger;
    settings = new QSettings;

    QDir(QDir::tempPath() + "/edah").removeRecursively();

    const QString fontFilename = utils->getDataDir() + "/OpenSans-Light.ttf";
    if(QFontDatabase::addApplicationFont(fontFilename) == -1)
    {
        LOG(QString("Couldn't load font \"%1\"").arg(fontFilename));
    }

    this->setGeometry(100, 100, 600, 338);
    this->restoreGeometry(settings->value("MainWindow_geometry").toByteArray());
    this->setMinimumSize(600, 338);

    QFile fstyle(":/style.qss");
    if(!fstyle.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG(QString("Couldn't open file \"%1\"").arg(fstyle.fileName()));
    }
    QString style = fstyle.readAll();
    this->setStyleSheet(style);

    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    this->setWindowFlags(Qt::FramelessWindowHint/* | Qt::WindowMinimizeButtonHint*/);
    this->grabGesture(Qt::TapGesture);

    centralWidget = new QWidget(this);
    this->setCentralWidget(this->centralWidget);
    {
        winFrame = new WinFrame(this->minimumSize(), centralWidget);
        winFrame->setObjectName("winFrame");
        winFrame->setCornerSize(QSize(20, 20));
        connect(winFrame, &WinFrame::geometryChanged, this, [this](QRect geom) {
            this->setGeometry(geom);
        });

        container = new QFrame(centralWidget);
        container->setObjectName("container");
        container->setLayout(new QVBoxLayout);
        container->layout()->setSpacing(0);
        container->layout()->setContentsMargins(QMargins());
        {
            //this->createTitleBar(container);
            //container->layout()->addWidget(titleBar);

            pluginContainer = new QLabel(container);
            pluginContainer->setObjectName("pluginContainer");
            pluginContainer->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
            pluginLayout = new QHBoxLayout;
            pluginLayout->setSpacing(0);
            pluginContainer->setLayout(pluginLayout);
            container->layout()->addWidget(pluginContainer);
        }
    }

    QString localeStr = settings->value("lang", "").toString();
    if(localeStr.isEmpty())
    {
        localeStr = QLocale::system().name().left(2);
    }
    if(localeStr != "en")
    {
        if(!translator.load(QLocale(localeStr), "lang", ".", ":/lang"))
        {
            LOG(QString("Couldn't load translation for \"%1\"").arg(localeStr));
        }
    }

    qApp->installTranslator(&translator);

#ifdef Q_OS_WIN
    globalSettings = new QSettings("HKEY_LOCAL_MACHINE\\Software\\Lukasz Matczak\\Edah", QSettings::NativeFormat);
    experimental = globalSettings->value("experimental", false).toBool();
    utils->setExperimental(experimental);
    updater = new Updater;
    updater->setInstallDir(QApplication::applicationDirPath());
    connect(this, &MainWindow::checkForUpdates, updater, &Updater::checkUpdates);
    connect(updater, &Updater::newUpdates, this, &MainWindow::showUpdate);
    updater->moveToThread(&updaterThread);
    updaterThread.start();
    emit checkForUpdates();

    if(settings->value("keepScreen", false).toBool())
    {
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED); // TODO: settings reload
    }
#else
    experimental = false;
#endif

    trayIcon = new QSystemTrayIcon(QIcon(":/img/icon.svg"), this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::showMenu);
    trayIcon->show();
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    SetThreadExecutionState(ES_CONTINUOUS);
#endif

    delete settings;
    delete logger;
    delete utils;
}

void MainWindow::showWindow() // TODO
{
    this->show();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    recalcSizes(e->size());
}

void MainWindow::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    recalcSizes(this->size());
}

void MainWindow::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        //minimizeBtn->setToolTip(tr("Minimize"));
        //closeBtn->setToolTip(tr("Close"));

        this->refreshPluginContainerText();
    }
    else
    {
        QMainWindow::changeEvent(e);
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    e->accept();

#ifdef Q_OS_WIN
    updaterThread.quit();
    updaterThread.wait();
#endif

    settings->setValue("MainWindow_geometry", this->saveGeometry());

    foreach(Plugin plugin, plugins)
    {
        this->unloadPlugin(&plugin);
    }

    qApp->quit();
}

void MainWindow::refreshPluginContainerText()
{
    if(plugins.size() == 0)
    {
        pluginContainer->setText(tr("There aren't any plugins selected!<br/>"
                                    "Go to: &#x2630; > Settings..."));
    }
    else
    {
        pluginContainer->setText("");
    }
}

bool MainWindow::loadPlugin(const QString &id, Plugin *plugin)
{
    plugin->id = id;
    plugin->loader = new QPluginLoader(utils->getPluginPath(id), this);
    plugin->plugin = qobject_cast<IPlugin*>(plugin->loader->instance());

    plugin->panel = nullptr;
    plugin->container = nullptr;

    if(!plugin->plugin)
    {
        LOG(utils->getPluginPath(id) + ": " + plugin->loader->errorString());
    }

    return plugin->plugin;
}

void MainWindow::unloadPlugin(Plugin *plugin)
{
    plugin->loader->unload();
    delete plugin->loader;
}

void MainWindow::reloadPlugins()
{
    int curr = 1;
    QStringList pluginsList = QDir(utils->getDataDir()+"/plugins").entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    const int enabledPlugins = pluginsList.size();

    emit loadProgressChanged(1, enabledPlugins+1);

    QVector<QString> newPluginsId;
    QVector<Plugin> newPlugins;

    // load new plugins
    for(int i=0; i<pluginsList.size(); i++)
    {
        QString pluginId = pluginsList[i];

        Plugin p;

        if(this->loadPlugin(pluginId, &p))
        {
            newPlugins.push_back(p);
            newPluginsId.push_back(pluginId);
        }
        curr++;
        emit loadProgressChanged(curr, enabledPlugins+1);
    }

    plugins.swap(newPlugins);

    // insert plugins' widgets to pluginLayout
    for(int i=0; i<plugins.size(); i++)
    {
        if(!plugins[i].plugin->hasPanel())
        {
            continue;
        }

        if(pluginLayout->count() > 0)
        {
            QWidget *line = new QWidget(this);
            line->setObjectName("line");
            line->setFixedWidth(2);
            pluginLayout->addWidget(line);
        }

        plugins[i].panel = plugins[i].plugin->panel();
        plugins[i].container = new QWidget(pluginContainer);
        plugins[i].container->setObjectName("plugin.container");
        plugins[i].container->setLayout(new QGridLayout);
        plugins[i].container->layout()->addWidget(plugins[i].panel);
        plugins[i].panel->setParent(plugins[i].container);

        pluginLayout->addWidget(plugins[i].container);
        plugins[i].panel->show();
    }

    this->recalcSizes(this->size());
}

void MainWindow::newProcess(const QString &message)
{
    // TODO
}

void MainWindow::recalcSizes(QSize size)
{
    QVector<QWidget*> vec;
    //vec.push_back(titleBar);
    vec.push_back(container);
    vec.push_back(pluginContainer);

    foreach(QWidget *widget, vec)
    {
        widget->setProperty("isMaximized", this->isMaximized());
        utils->updateStyle(widget);
    }

    if(this->isMaximized())
    {
        winFrame->hide();
        container->setGeometry(0, 0, size.width(), size.height());
    }
    else
    {
        winFrame->setGeometry(0, 0, size.width(), size.height());
        winFrame->show();

        container->setGeometry(3, 4, size.width()-6, size.height()-6);
    }

    /*titleLbl->setStyleSheet(QString("color: qlineargradient(spread:pad, x1:0, y1:0, x2:%1, y2:0,"
                            "stop:0 rgba(255, 255, 255, 255),"
                            "stop:0.95 rgba(255,255,255,255),"
                            "stop:1 rgba(0, 0, 0, 0));")
                            .arg(titleLbl->width()));*/

    int height = size.height()/16;
    int fontSize = height/1.5f;

    pluginContainer->setStyleSheet(QString("#pluginContainer { font-size: %1px; }")
                                   .arg(fontSize));

    this->refreshPluginContainerText();

    int visiblePluginsCount = 0;
    for(int i=0; i<plugins.size(); i++)
    {
        if(plugins[i].plugin->hasPanel())
            visiblePluginsCount++;
    }

    int width = pluginContainer->contentsRect().width()/visiblePluginsCount;

    for(int i=0; i<plugins.size(); i++)
    {
        if(!plugins[i].plugin->hasPanel())
        {
            continue;
        }

        int margin = qMax(0.0, width*2.6666 - pluginContainer->height())*0.75;
        margin = 0;
        plugins[i].container->layout()->setContentsMargins(margin, 0, margin, 0);
        plugins[i].container->setFixedWidth(width);
        plugins[i].container->setFixedHeight(qMin<int>(width*2.6666, pluginContainer->height()));
        plugins[i].container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
}

void MainWindow::settingsChanged()
{
    QLocale locale = QLocale(settings->value("lang", "").toString());
    translator.load(locale, "lang", ".", ":/lang");

    for(int i=0; i<plugins.size(); i++)
    {
        plugins[i].plugin->settingsChanged();
    }
}

void MainWindow::onMaximizeBtnClicked()
{
    if(this->isMaximized())
        this->showNormal();
    else
        this->showMaximized();
}

void MainWindow::showMenu(QSystemTrayIcon::ActivationReason reason)
{
    if(reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::Context)
        return;

    QMenu *menu = new QMenu(this);

    menu->addAction(tr("Settings..."), this, SLOT(showSettings()));

    if(this->updateAvailable)
    {
        menu->addAction(tr("Update is available to download!"), this, SLOT(showUpdateDialog()));
    }

    menu->addAction(tr("About..."), this, SLOT(showAbout()));
    menu->addAction(tr("Close"), this, &MainWindow::close);

    menu->popup(QCursor::pos());
}

void MainWindow::showAbout()
{
    AboutDialog *dialog = new AboutDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::showUpdateDialog()
{
#ifdef Q_OS_WIN
    UpdateDialog *dlg = new UpdateDialog(&updateInfo, updater);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
#endif
}

void MainWindow::showSettings()
{
    Settings *settings = new Settings(&plugins);
    settings->setAttribute(Qt::WA_DeleteOnClose);
    connect(settings, &Settings::settingsChanged, this, &MainWindow::settingsChanged);
    settings->exec();
}

void MainWindow::showUpdate(UpdateInfoArray info)
{
    disconnect(updater, &Updater::newUpdates, this, &MainWindow::showUpdate);

    if(info.size() > 0)
    {
        this->updateAvailable = true;
        this->updateInfo = info;

        //menuBtn->setIcon(QIcon(":/img/menu_info.svg")); // TODO: tray badge
    }
}
