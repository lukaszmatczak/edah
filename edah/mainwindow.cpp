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
#include "osutils.h"
#include "logger.h"
#include "database.h"
#include "mypushbutton.h"
#include "aboutdialog.h"
#include "settings.h"

#ifdef Q_OS_LINUX
#include "linuxutils.h"
#endif
#ifdef Q_OS_WIN
#include "windowsutils.h"
#endif

#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QFontDatabase>
#include <QTime>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QSqlQuery>

#include <QTimeLine>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

#include <QMenu>
#include <QMessageBox>

#include <QDebug>

OSUtils *utils;
Logger *logger;
Database *db;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
#ifdef Q_OS_LINUX
    utils = new LinuxUtils;
#endif
#ifdef Q_OS_WIN
    utils = new WindowsUtils;
#endif

    logger = new Logger;
    db = new Database;

    QFontDatabase::addApplicationFont(utils->getDataDir() + "/OpenSans-Light.ttf");

    this->setGeometry(100, 100, 400, 300);

    this->restoreGeometry(db->getValue("MainWindow_geometry").toByteArray());

    this->setMinimumSize(200, 100);

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

            pluginContainer = new QWidget(container);
            pluginContainer->setObjectName("pluginContainer");
            pluginLayout = new QHBoxLayout;
            pluginContainer->setLayout(pluginLayout);
            pluginLayout->setSpacing(0);
            container->layout()->addWidget(pluginContainer);

            this->createBottomBar(container);
            container->layout()->addWidget(bottomBar);
        }
    }

    translator = new QTranslator(this);
    QLocale locale = QLocale(QLocale::system().name().left(2));
    QString localeStr = db->getValue("lang", "").toString();
    if(!localeStr.isEmpty())
    {
        locale = QLocale(localeStr);
    }
    translator->load(locale, "lang", ".", ":/lang");
    qApp->installTranslator(translator);

    if(db->getValue("fullscreen", false).toBool())
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

    this->loadPlugins();
}

bool MainWindow::loadPlugin(const QString &id, Plugin *plugin)
{
    plugin->id = id;
    plugin->loader = new QPluginLoader(utils->getPluginPath(id), this);
    plugin->plugin = qobject_cast<IPlugin*>(plugin->loader->instance());

    if(!plugin->plugin)
    {
        LOG(QString("Couldn't load plugin \"%1\"").arg(utils->getPluginPath(id)));
    }

    return plugin->plugin;
}

void MainWindow::loadPlugins()
{
    db->db.exec("CREATE TABLE IF NOT EXISTS plugins (`id` INTEGER PRIMARY KEY AUTOINCREMENT, `plugin_id` TEXT, `order` INTEGER, `enabled` INTEGER)");

    QSqlQuery q(db->db);
    q.prepare("SELECT `plugin_id` FROM `plugins` WHERE `enabled`=1 ORDER BY `order`");
    q.exec();

    while(q.next())
    {
        Plugin plugin;

        if(this->loadPlugin(q.value(0).toString(), &plugin))
        {
            plugins.push_back(plugin);
        }
    }

    for(int i=0; i<plugins.size(); i++)
    {
        if(i!=0)
        {
            QWidget *line = new QWidget(this);
            line->setObjectName("line");
            line->setFixedWidth(2);
            pluginLayout->addWidget(line);
        }

        if(i==0)
        {
            QWidget *small = plugins[i].plugin->getSmallWidget();
            small->setParent(pluginContainer);
            small->hide();

            plugins[i].widget = plugins[i].plugin->getBigWidget();
            plugins[i].isBig = true;
        }
        else
        {
            QWidget *big = plugins[i].plugin->getBigWidget();
            big->setParent(pluginContainer);
            big->hide();

            plugins[i].widget = plugins[i].plugin->getSmallWidget();
            plugins[i].isBig = false;
        }

        plugins[i].widget->setParent(pluginContainer);
        pluginLayout->addWidget(plugins[i].widget);
    }

    activePlugin = 0;
    this->changeActivePlugin(0);

}

void MainWindow::fadeInOut(QWidget *w1, QWidget *w2, int duration, int start, int stop)
{
    QTimeLine *timeLine = new QTimeLine(duration);
    QGraphicsOpacityEffect *effect1 = new QGraphicsOpacityEffect;
    QGraphicsOpacityEffect *effect2 = new QGraphicsOpacityEffect;

    effect1->setOpacity(start/255.0);
    effect2->setOpacity(start/255.0);
    w1->setGraphicsEffect(effect1);
    w2->setGraphicsEffect(effect2);

    timeLine->setFrameRange(start, stop);
    connect(timeLine, &QTimeLine::frameChanged, this, [effect1, effect2](int frame) {
        const float opacity = frame/255.0;
        effect1->setOpacity(opacity);
        effect2->setOpacity(opacity);
    });
    timeLine->start();

    QEventLoop loop;
    connect(timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void MainWindow::changeActivePlugin(int pluginIdx)
{

    for(int i=0; i<plugins.size(); i++)
    {
        plugins[i].isBig = true;
    }

    if(activePlugin != pluginIdx)
    {
        this->fadeInOut(plugins[activePlugin].widget, plugins[pluginIdx].widget, 250, 255, 0);

        QTimeLine *timeLine = new QTimeLine(350);
        timeLine->setEasingCurve(QEasingCurve::InOutSine);
        timeLine->setFrameRange(0, 100);

        int smallWidth = plugins[pluginIdx].widget->width();
        int bigWidth = plugins[activePlugin].widget->width();

        plugins[activePlugin].widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        plugins[pluginIdx].widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

        connect(timeLine, &QTimeLine::frameChanged, this, [this, pluginIdx, smallWidth, bigWidth](int frame) {
            plugins[activePlugin].widget->setFixedWidth(smallWidth + (bigWidth-smallWidth)*(100.0f-frame)/100.0f);
            plugins[pluginIdx].widget->setFixedWidth(smallWidth + (bigWidth-smallWidth)*(frame/100.0f));
        });
        timeLine->start();

        QEventLoop loop;
        connect(timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
        loop.exec();

        plugins[pluginIdx].widget->hide();
        pluginLayout->removeWidget(plugins[pluginIdx].widget);
        QWidget *newBigWidget = plugins[pluginIdx].plugin->getBigWidget();
        newBigWidget->setParent(pluginContainer);
        pluginLayout->insertWidget(pluginIdx*2, newBigWidget);
        newBigWidget->setFixedWidth(bigWidth);
        newBigWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        newBigWidget->show();
        plugins[pluginIdx].widget = newBigWidget;

        plugins[activePlugin].widget->hide();
        pluginLayout->removeWidget(plugins[activePlugin].widget);
        QWidget *newSmallWidget = plugins[activePlugin].plugin->getSmallWidget();
        newSmallWidget->setParent(pluginContainer);
        pluginLayout->insertWidget(activePlugin*2, newSmallWidget);
        newSmallWidget->setMinimumWidth(1);
        newSmallWidget->setMaximumWidth(32768);
        newSmallWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        newSmallWidget->show();
        plugins[activePlugin].widget = newSmallWidget;

        this->fadeInOut(plugins[activePlugin].widget, plugins[pluginIdx].widget, 250, 0, 255);
    }

    for(int i=0; i<plugins.size(); i++)
    {
        plugins[i].isBig = (i == pluginIdx);
    }

    activePlugin = pluginIdx;
}

void MainWindow::refreshPlugins()
{
    QVector<QString> pluginsId;
    QVector<Plugin> newPlugins;

    QSqlQuery q(db->db);
    q.prepare("SELECT `plugin_id` FROM `plugins` WHERE `enabled`=1 ORDER BY `order`");
    q.exec();

    while(q.next())
    {
        QString pluginId = q.value(0).toString();

        int idx = -1;
        for(int i=0; i<plugins.size(); i++)
        {
            if(plugins[i].id == pluginId)
            {
                idx = i;
                break;
            }
        }

        if(idx != -1)
        {
            newPlugins.push_back(plugins[idx]);
        }
        else
        {
            Plugin plugin;

            if(this->loadPlugin(pluginId, &plugin))
            {
                newPlugins.push_back(plugin);
            }
        }

        pluginsId.push_back(pluginId);
    }

    QMap<QString, QWidget*> pluginsWidgets;
    for(int i=0; i<plugins.size(); i++)
    {
        if(pluginsId.contains(plugins[i].id))
        {
            pluginsWidgets.insert(plugins[i].id, pluginLayout->itemAt(i*2)->widget());
        }
    }

    for(int i=0; i<plugins.size(); i++)
    {
        if(!pluginsId.contains(plugins[i].id))
        {
            plugins[i].loader->unload();
        }
    }

    plugins = newPlugins;

    QLayoutItem *i;
    while((i = pluginLayout->takeAt(0)) != 0)
    {
        if(i->widget()->objectName() == "line")
        {
            delete i->widget();
        }
        delete i;
    }

    for(int i=0; i<plugins.size(); i++)
    {
        if(i!=0)
        {
            QWidget *line = new QWidget(this);
            line->setObjectName("line");
            line->setFixedWidth(2);
            //lines.push_back(line);
            pluginLayout->addWidget(line);
        }

        if(pluginsWidgets.contains(plugins[i].id))
        {
            pluginLayout->addWidget(pluginsWidgets[plugins[i].id]);
        }
        else
        {
            pluginLayout->addWidget(plugins[i].plugin->getBigWidget());
        }
    }
}

MainWindow::~MainWindow()
{
    if(!this->isFullScreen())
    {
        db->setValue("MainWindow_geometry", this->saveGeometry());
    }

    foreach (Plugin plugin, plugins)
    {
        plugin.loader->unload();
    }

    delete db;
    delete logger;
    delete utils;
}

void MainWindow::addShadowEffect(QWidget *widget, QColor color)
{
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(30);
    effect->setColor(color);
    effect->setOffset(0,0);
    widget->setGraphicsEffect(effect);
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
    updateStyle(winFrame);

    titleBar->setProperty("hasFocus", this->isActiveWindow());
    updateStyle(titleBar);

    QGraphicsDropShadowEffect *shadow = dynamic_cast<QGraphicsDropShadowEffect*>(titleBar->graphicsEffect());
    if(shadow)
    {
        shadow->setBlurRadius(this->isActiveWindow() ? 30 : 0);
    }
}

void MainWindow::showEvent(QShowEvent *e)
{
    recalcSizes(this->size());
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    maximizeBtn->setToolTip(this->isMaximized() ? tr("Restore") : tr("Maximize"));

    recalcSizes(e->size());
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
        if(!plugins[i].isBig && plugins[i].widget->underMouse())
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

void MainWindow::updateStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void MainWindow::recalcSizes(QSize size)
{
    QVector<QWidget*> vec;
    vec.push_back(titleBar);
    vec.push_back(container);
    vec.push_back(pluginContainer);
    //foreach(QWidget *widget, QVector<QWidget*>({titleBar, container, pluginContainer}))
    foreach (QWidget *widget, vec)
    {
        widget->setProperty("isMaximized", this->isMaximized() || this->isFullScreen());
        updateStyle(widget);
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

    clockLbl->setStyleSheet(QString("font-size: %1px")
                            .arg((int)(height/1.5f)));

    for(int i=0; i<plugins.size(); i++)
    {
        int count = plugins.size();
        int width = pluginContainer->contentsRect().width()/(count+3);
        if(plugins[i].isBig)
        {
            plugins[i].widget->setFixedWidth(width*4);
            plugins[i].widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        }
        else
        {
            plugins[i].widget->setMaximumWidth(32768);
            plugins[i].widget->setMinimumWidth(1);
            plugins[i].widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        }
    }
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
    this->addShadowEffect(titleLbl, Qt::white);
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

void MainWindow::showSettings()
{
    Settings *settings = new Settings(&plugins);
    settings->setAttribute(Qt::WA_DeleteOnClose);
    connect(settings, &Settings::settingsChanged, this, &MainWindow::settingsChanged);
    settings->exec();
}

void MainWindow::settingsChanged()
{
    QLocale locale = QLocale(db->getValue("lang", "").toString());
    translator->load(locale, "lang", ".", ":/lang");

    bool fullscreen = db->getValue("fullscreen", false).toBool();
    if(fullscreen != this->isFullScreen())
    {
        titleBar->setVisible(!fullscreen);
        bottomBar->setVisible(fullscreen);
        fullscreen ? this->showFullScreen() : this->showNormal();
    }

    this->refreshPlugins();
}

void MainWindow::showAbout()
{
    AboutDialog *dialog = new AboutDialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}
