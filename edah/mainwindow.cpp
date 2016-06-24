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

#include "mainwindow.h"
#include "aboutdialog.h"
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

#include <QDebug>

QDataStream &operator<<(QDataStream &stream, const PluginCfgEntry &entry)
{
    stream << entry.enabled << entry.id;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, PluginCfgEntry &entry)
{
    stream >> entry.enabled >> entry.id;

    return stream;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), activePlugin(-1)
{
    QCoreApplication::setOrganizationName("Lukasz Matczak");
    QCoreApplication::setApplicationName("Edah");

    utils = new Utils;
    logger = new Logger;
    settings = new QSettings;

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
            this->createTitleBar(container);
            container->layout()->addWidget(titleBar);

            pluginContainer = new QLabel(container);
            pluginContainer->setObjectName("pluginContainer");
            pluginContainer->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
            pluginLayout = new QHBoxLayout;
            pluginLayout->setSpacing(0);
            pluginContainer->setLayout(pluginLayout);
            container->layout()->addWidget(pluginContainer);

            this->createBottomBar(container);
            container->layout()->addWidget(bottomBar);
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

    if(settings->value("fullscreen", false).toBool())
    {
        titleBar->setVisible(false);
        this->showFullScreen();
    }
    else
    {
        bottomBar->setVisible(false);
    }

    connect(&timer, &QTimer::timeout, this, &MainWindow::timerSlot);
    timer.start(100);

    connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

    this->reloadPlugins();
}

MainWindow::~MainWindow()
{
    if(!this->isFullScreen())
    {
        settings->setValue("MainWindow_geometry", this->saveGeometry());
    }

    foreach(Plugin plugin, plugins)
    {
        this->unloadPlugin(&plugin);
    }

    delete settings;
    delete logger;
    delete utils;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    maximizeBtn->setToolTip(this->isMaximized() ? tr("Restore") : tr("Maximize"));

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
        menuBtn->setToolTip(tr("Menu"));
        minimizeBtn->setToolTip(tr("Minimize"));
        maximizeBtn->setToolTip(this->isMaximized() ? tr("Restore") : tr("Maximize"));
        closeBtn->setToolTip(tr("Close"));
        closeBtn_bottom->setToolTip(tr("Close"));
        minimizeBtn_bottom->setToolTip(tr("Minimize"));
        menuBtn_bottom->setToolTip(tr("Menu"));

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
    else
    {
        QMainWindow::changeEvent(e);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    movePos = e->pos();

    for(int i=0; i<plugins.size(); i++)
    {
        if(plugins[i].plugin->hasPanel() && plugins[i].container->underMouse())
        {
            this->changeActivePlugin(i);
            break;
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) &&
        this->titleBar->geometry().contains(movePos))
    {
        QPoint diff = e->pos() - movePos;
        QPoint newpos = this->pos() + diff;

        this->move(newpos);
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) &&
        this->titleBar->geometry().contains(e->pos()))
    {
        this->onMaximizeBtnClicked();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if((e->key() >= Qt::Key_F1) && (e->key() <= Qt::Key_F35))
    {
        int key = e->key()-Qt::Key_F1;

        int idx = 0;
        for(int i=0; i<plugins.size(); i++)
        {
            if(plugins[i].plugin->hasPanel())
            {
                if(key == idx)
                {
                    this->changeActivePlugin(i);
                    break;
                }
                idx++;
            }
        }
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
        LOG(plugin->loader->errorString());
    }

    return plugin->plugin;
}

void MainWindow::unloadPlugin(Plugin *plugin)
{
    plugin->loader->unload();
    delete plugin->loader;
}

bool MainWindow::findPlugin(const QString &id, Plugin *plugin)
{
    for(int i=0; i<plugins.size(); i++)
    {
        if(plugins[i].id == id)
        {
            *plugin = plugins[i];
            return true;
        }
    }

    return false;
}

void MainWindow::reloadPlugins()
{
    QVector<QString> newPluginsId;
    QVector<Plugin> newPlugins;

    // delete all items in pluginLayout
    QLayoutItem *i;
    while((i = pluginLayout->takeAt(0)) != 0)
    {
        i->widget()->hide();
        if(i->widget()->objectName() == "line")
        {
            delete i->widget();
        }
        else if(i->widget()->objectName() == "plugin.container")
        {
            QObjectList children = i->widget()->children();
            for(int i=0; i<children.size(); i++)
            {
                children[i]->setParent(this);
            }
            delete i->widget();
        }
        delete i;
    }

    // load new plugins
    QVector<PluginCfgEntry> cfg;
    QByteArray arr = settings->value("plugins").toByteArray();
    QDataStream stream(arr);
    stream >> cfg;

    for(int i=0; i<cfg.size(); i++)
    {
        if(!cfg[i].enabled)
            continue;

        QString pluginId = cfg[i].id;

        Plugin p;

        if(this->findPlugin(pluginId, &p) || this->loadPlugin(pluginId, &p))
        {
            newPlugins.push_back(p);
            newPluginsId.push_back(pluginId);
        }
    }

    // select new big widget
    if(activePlugin != -1)
    {
        activePlugin = newPluginsId.indexOf(plugins[activePlugin].id);
    }
    if(activePlugin == -1)
    {
        for(int i=0; i<newPlugins.size(); i++)
        {
            if(newPlugins[i].plugin->hasPanel())
            {
                activePlugin = i;
                break;
            }
        }
    }

    // unload old plugins
    for(int i=0; i<plugins.size(); i++)
    {
        if(!newPluginsId.contains(plugins[i].id))
        {
            this->unloadPlugin(&plugins[i]);
        }
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

        plugins[i].panel = (i == activePlugin) ?
                    plugins[i].plugin->bigPanel() :
                    plugins[i].plugin->smallPanel();
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

void MainWindow::changeActivePlugin(int pluginIdx)
{
    // prevent changing active plugin during another change
    static bool isDuringChanging;

    if(isDuringChanging)
    {
        return;
    }

    isDuringChanging = true;

    if(activePlugin != pluginIdx)
    {
        utils->fadeInOut(plugins[activePlugin].panel,
                         plugins[pluginIdx].panel,
                         250, 255, 0);

        int smallWidth = plugins[pluginIdx].container->width();
        int bigWidth = plugins[activePlugin].container->width();

        plugins[pluginIdx].panel->hide();
        plugins[activePlugin].panel->hide();

        plugins[pluginIdx].panel = plugins[pluginIdx].plugin->bigPanel();
        plugins[pluginIdx].panel->hide();
        plugins[pluginIdx].panel->setParent(plugins[pluginIdx].container);
        plugins[pluginIdx].container->layout()->addWidget(plugins[pluginIdx].panel);

        plugins[activePlugin].panel = plugins[activePlugin].plugin->smallPanel();
        plugins[activePlugin].panel->hide();
        plugins[activePlugin].panel->setParent(plugins[activePlugin].container);
        plugins[activePlugin].container->layout()->addWidget(plugins[activePlugin].panel);

        QTimeLine timeLine(350);
        timeLine.setEasingCurve(QEasingCurve::InOutSine);
        timeLine.setFrameRange(smallWidth, bigWidth);

        plugins[activePlugin].container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        plugins[pluginIdx].container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

        connect(&timeLine, &QTimeLine::frameChanged, this, [this, pluginIdx, smallWidth, bigWidth](int frame) {
            plugins[activePlugin].container->setFixedWidth(bigWidth-(frame-smallWidth));
            plugins[pluginIdx].container->setFixedWidth(frame);
        });
        timeLine.start();

        QEventLoop loop;
        connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
        loop.exec();

        plugins[activePlugin].panel->show();
        plugins[pluginIdx].panel->show();

        utils->fadeInOut(plugins[activePlugin].panel,
                         plugins[pluginIdx].panel,
                         250, 0, 255);
    }

    plugins[pluginIdx].container->setFocus();

    activePlugin = pluginIdx;

    isDuringChanging = false;
}

void MainWindow::createTitleBar(QWidget *parent)
{
    titleBar = new QFrame(parent);
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(32);

    titleBar->setLayout(new QHBoxLayout);
    titleBar->layout()->setMargin(0);
    titleBar->layout()->setContentsMargins(8, 4, 8, 4);

    menuBtn = new QToolButton(titleBar);
    menuBtn->setIcon(QIcon(":/img/menu.svg"));
    menuBtn->setFixedSize(24, 24);
    menuBtn->setObjectName("menuBtn");
    connect(menuBtn, &QToolButton::clicked, this, &MainWindow::showMenu);
    titleBar->layout()->addWidget(menuBtn);

    QLabel *iconLbl = new QLabel(titleBar);
    iconLbl->setPixmap(QPixmap(":/img/icon.svg"));
    titleBar->layout()->addWidget(iconLbl);

    this->setWindowTitle("Edah");
    titleLbl = new QLabel("Edah", titleBar);
    utils->addShadowEffect(titleLbl, Qt::white);
    ((QHBoxLayout*)titleBar->layout())->addWidget(titleLbl, 1);

    minimizeBtn = new QToolButton(titleBar);
    minimizeBtn->setIcon(QIcon(":/img/minimize.svg"));
    minimizeBtn->setFixedSize(24, 24);
    minimizeBtn->setObjectName("minimizeBtn");
    connect(minimizeBtn, &QToolButton::clicked, this, &MainWindow::showMinimized);
    titleBar->layout()->addWidget(minimizeBtn);

    maximizeBtn = new QToolButton(titleBar);
    maximizeBtn->setIcon(QIcon(":/img/maximize.svg"));
    maximizeBtn->setFixedSize(24, 24);
    maximizeBtn->setObjectName("maximizeBtn");
    connect(maximizeBtn, &QToolButton::clicked, this, &MainWindow::onMaximizeBtnClicked);
    titleBar->layout()->addWidget(maximizeBtn);

    closeBtn = new QToolButton(titleBar);
    closeBtn->setIcon(QIcon(":/img/close.svg"));
    closeBtn->setFixedSize(24, 24);
    closeBtn->setObjectName("closeBtn");
    connect(closeBtn, &QToolButton::clicked, this, &MainWindow::close);
    titleBar->layout()->addWidget(closeBtn);
}

void MainWindow::createBottomBar(QWidget *parent)
{
    bottomBar = new QFrame(parent);
    bottomBar->setObjectName("bottomBar");

    bottomBar->setLayout(new QHBoxLayout);
    bottomBar->layout()->setSpacing(0);
    bottomBar->layout()->setContentsMargins(QMargins());

    ((QHBoxLayout*)bottomBar->layout())->addStretch(1);

    closeBtn_bottom = new QToolButton(bottomBar);
    closeBtn_bottom->setIcon(QIcon(":/img/close.svg"));
    closeBtn_bottom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    closeBtn_bottom->setObjectName("closeBtn_bottom");
    connect(closeBtn_bottom, &QToolButton::clicked, this, &MainWindow::close);
    bottomBar->layout()->addWidget(closeBtn_bottom);

    minimizeBtn_bottom = new QToolButton(bottomBar);
    minimizeBtn_bottom->setIcon(QIcon(":/img/minimize.svg"));
    minimizeBtn_bottom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    minimizeBtn_bottom->setObjectName("minimizeBtn_bottom");
    connect(minimizeBtn_bottom, &QToolButton::clicked, this, &MainWindow::showMinimized);
    bottomBar->layout()->addWidget(minimizeBtn_bottom);

    menuBtn_bottom = new QToolButton(bottomBar);
    menuBtn_bottom->setIcon(QIcon(":/img/menu.svg"));
    menuBtn_bottom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    menuBtn_bottom->setObjectName("menuBtn_bottom");
    connect(menuBtn_bottom, &QToolButton::clicked, this, &MainWindow::showMenu);
    bottomBar->layout()->addWidget(menuBtn_bottom);

    ((QHBoxLayout*)bottomBar->layout())->addStretch(12);

    clockLbl = new QLabel("--:--", bottomBar);

    bottomBar->layout()->addWidget(clockLbl);

    ((QHBoxLayout*)bottomBar->layout())->addStretch(1);
}

void MainWindow::newProcess(const QString &message)
{
    // TODO
}

void MainWindow::timerSlot()
{
    clockLbl->setText(QTime::currentTime().toString(tr("hh:mm AP")));
}

void MainWindow::onFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old); Q_UNUSED(now);

    winFrame->setProperty("hasFocus", this->isActiveWindow());
    utils->updateStyle(winFrame);

    titleBar->setProperty("hasFocus", this->isActiveWindow());
    utils->updateStyle(titleBar);

    QGraphicsDropShadowEffect *shadow = dynamic_cast<QGraphicsDropShadowEffect*>(titleBar->graphicsEffect());
    if(shadow)
    {
        shadow->setBlurRadius(this->isActiveWindow() ? 30 : 0);
    }
}

void MainWindow::recalcSizes(QSize size)
{
    QVector<QWidget*> vec;
    vec.push_back(titleBar);
    vec.push_back(container);
    vec.push_back(pluginContainer);

    foreach(QWidget *widget, vec)
    {
        widget->setProperty("isMaximized", this->isMaximized() || this->isFullScreen());
        utils->updateStyle(widget);
    }

    if(this->isMaximized() || this->isFullScreen())
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

    titleLbl->setStyleSheet(QString("color: qlineargradient(spread:pad, x1:0, y1:0, x2:%1, y2:0,"
                            "stop:0 rgba(255, 255, 255, 255),"
                            "stop:0.95 rgba(255,255,255,255),"
                            "stop:1 rgba(0, 0, 0, 0));")
                            .arg(titleLbl->width()));

    int height = size.height()*this->logicalDpiY()/2048;
    bottomBar->setFixedHeight(height);
    QSize iconSize = QSize(height*2, height/2);
    closeBtn_bottom->setIconSize(iconSize);
    minimizeBtn_bottom->setIconSize(iconSize);
    menuBtn_bottom->setIconSize(iconSize);

    int fontSize = height/1.5f;
    clockLbl->setStyleSheet(QString("font-size: %1px")
                            .arg(fontSize));

    pluginContainer->setStyleSheet(QString("#pluginContainer { font-size: %1px; }")
                                   .arg(fontSize));

    if(plugins.size() == 0)
    {
        pluginContainer->setText(tr("There aren't any plugins selected!<br/>"
                                    "Go to: &#x2630; > Settings..."));
    }
    else
    {
        pluginContainer->setText("");
    }

    int visiblePluginsCount = 0;
    for(int i=0; i<plugins.size(); i++)
    {
        if(plugins[i].plugin->hasPanel())
            visiblePluginsCount++;
    }

    int width = pluginContainer->contentsRect().width()/(visiblePluginsCount+3);

    for(int i=0; i<plugins.size(); i++)
    {
        if(!plugins[i].plugin->hasPanel())
        {
            continue;
        }

        if(i == activePlugin)
        {
            int margin = qMax(0.0, width*2.6666 - pluginContainer->height())*0.75;
            plugins[i].container->layout()->setContentsMargins(margin, 0, margin, 0);
            plugins[i].container->setFixedWidth(width*4);
            plugins[i].container->setFixedHeight(qMin<int>(width*2.6666, pluginContainer->height()));
            plugins[i].container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        else
        {
            plugins[i].container->setMaximumWidth(32768);
            plugins[i].container->setMinimumWidth(1);
            plugins[i].container->setFixedHeight(qMin<int>(width*2.6666, pluginContainer->height()));
            plugins[i].container->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        }
    }
}

void MainWindow::settingsChanged()
{
    QLocale locale = QLocale(settings->value("lang", "").toString());
    translator.load(locale, "lang", ".", ":/lang");

    bool fullscreen = settings->value("fullscreen", false).toBool();
    if(fullscreen != this->isFullScreen())
    {
        titleBar->setVisible(!fullscreen);
        bottomBar->setVisible(fullscreen);
        fullscreen ? this->showFullScreen() : this->showNormal();
    }

    this->reloadPlugins();
}

void MainWindow::onMaximizeBtnClicked()
{
    if(this->isMaximized())
        this->showNormal();
    else
        this->showMaximized();
}

void MainWindow::showMenu()
{
    QMenu *menu = new QMenu(this);

    menu->addAction(tr("Settings..."), this, SLOT(showSettings()));
    menu->addAction(tr("About..."), this, SLOT(showAbout()));

    if(QObject::sender() == menuBtn_bottom)
    {
        menu->setStyleSheet(QString("font-size: %1px")
                            .arg(this->height()/32));

        menu->popup(menuBtn_bottom->mapToGlobal(QPoint(0, -menu->sizeHint().height())));
    }
    else
    {
        menu->popup(menuBtn->mapToGlobal(QPoint(0, menuBtn->height())));
    }
}

void MainWindow::showAbout()
{
    AboutDialog *dialog = new AboutDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::showSettings()
{
    Settings *settings = new Settings(&plugins);
    settings->setAttribute(Qt::WA_DeleteOnClose);
    connect(settings, &Settings::settingsChanged, this, &MainWindow::settingsChanged);
    settings->exec();
}
