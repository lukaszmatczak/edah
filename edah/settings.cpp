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

#include "settings.h"
#include "updatedialog.h"

#include <libedah/utils.h>

#include <QVBoxLayout>
#include <QTabWidget>

#include <QPushButton>
#include <QFormLayout>
#include <QStandardItem>
#include <QEvent>
#include <QApplication>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonArray>

#include <QDebug>

////////////////
/// Settings ///
////////////////

Settings::Settings(QVector<Plugin> *plugins, Updater *updater) : plugins(plugins)
{
    this->setLayout(new QVBoxLayout);

    tabs = new QTabWidget(this);

    generalTab = new GeneralTab(updater);
    tabs->addTab(generalTab, "General");
    generalTab->loadSettings();

    foreach(Plugin p, *plugins)
    {
        QWidget *t = p.plugin->getSettingsTab();
        if(t)
        {
            tabs->addTab(t, p.plugin->getPluginName());
            p.plugin->loadSettings();
        }
    }

    this->layout()->addWidget(tabs);

    dialogBtns = new QDialogButtonBox(QDialogButtonBox::Ok |
                                      QDialogButtonBox::Cancel |
                                      QDialogButtonBox::Apply, this);

    connect(dialogBtns, &QDialogButtonBox::accepted, this, [this]() {
        this->writeSettings();
        this->close();
    });
    connect(dialogBtns, &QDialogButtonBox::rejected, this, &Settings::close);
    connect(dialogBtns->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &Settings::writeSettings);

    this->layout()->addWidget(dialogBtns);

    QT_TRANSLATE_NOOP("QPlatformTheme", "Cancel");
    QT_TRANSLATE_NOOP("QPlatformTheme", "Apply");
    QT_TRANSLATE_NOOP("QPlatformTheme", "OK");

    QEvent langEvent(QEvent::LanguageChange);
    this->changeEvent(&langEvent);
}

Settings::~Settings()
{
    int count = tabs->count()-1;

    for(int i=0; i<count; i++)
    {
        tabs->widget(1)->setParent(0);
    }
}

void Settings::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        this->setWindowTitle(tr("Settings"));

        tabs->setTabText(0, tr("General"));
    }
}

void Settings::writeSettings()
{
    generalTab->writeSettings();
    foreach(Plugin p, *plugins)
    {
        p.plugin->writeSettings();
    }

    emit settingsChanged();

    foreach(Plugin p, *plugins)
    {
        p.plugin->loadSettings();
    }

    // TODO: reload tabs
}

//////////////////
/// GeneralTab ///
//////////////////

GeneralTab::GeneralTab(Updater *updater) : updater(updater)
{
    QFormLayout *layout = new QFormLayout;
    this->setLayout(layout);

    langLbl = new QLabel(this);

    langBox = new QComboBox(this);
    langBox->addItem("English", "en");
    langBox->addItem("polski", "pl");

    layout->addRow(langLbl, langBox);

    fullscreenChk = new QCheckBox(this);
    layout->addRow(fullscreenChk);

    autostart = new QSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    autostartChk = new QCheckBox(this);
    layout->addRow(autostartChk);

    QHBoxLayout *pluginsLayout = new QHBoxLayout;
    layout->addRow(pluginsLayout);

    QVBoxLayout *installedPluginsLayout = new QVBoxLayout;
    pluginsLayout->addLayout(installedPluginsLayout);

    installedPluginsLbl = new QLabel(this);
    installedPluginsLayout->addWidget(installedPluginsLbl);

    pluginsTbl = new QTableView(this);
    pluginsModel = new PluginTableModel;

    pluginsTbl->setModel(pluginsModel);

    pluginsTbl->horizontalHeader()->hide();
    pluginsTbl->verticalHeader()->hide();
    pluginsTbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    pluginsTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    pluginsTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginsTbl->setShowGrid(false);
    pluginsTbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    pluginsTbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    pluginsTbl->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    pluginsTbl->setWordWrap(false);
    connect(pluginsTbl, &QTableView::pressed, this, &GeneralTab::installedPluginSelected);
    installedPluginsLayout->addWidget(pluginsTbl);

    QVBoxLayout *availPluginsLayout = new QVBoxLayout;
    pluginsLayout->addLayout(availPluginsLayout);

    QHBoxLayout *pluginsBtnsLayout = new QHBoxLayout;
    installedPluginsLayout->addLayout(pluginsBtnsLayout);

    moveUpBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_ArrowUp), "", this);
    connect(moveUpBtn, &QPushButton::clicked, this, &GeneralTab::moveUpBtnClicked);
    pluginsBtnsLayout->addWidget(moveUpBtn);

    moveDownBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_ArrowDown), "", this);
    connect(moveDownBtn, &QPushButton::clicked, this, &GeneralTab::moveDownBtnClicked);
    pluginsBtnsLayout->addWidget(moveDownBtn);

    availPluginsLbl = new QLabel(this);
    availPluginsLayout->addWidget(availPluginsLbl);

    availPluginsTbl = new QTableView(this);
    availPluginsModel = new AvailPluginTableModel;

    availPluginsTbl->setModel(availPluginsModel);
    availPluginsTbl->horizontalHeader()->hide();
    availPluginsTbl->verticalHeader()->hide();
    availPluginsTbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    availPluginsTbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    availPluginsTbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    availPluginsTbl->setShowGrid(false);
    availPluginsTbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    availPluginsTbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    availPluginsTbl->setWordWrap(false);
    connect(availPluginsTbl, &QTableView::pressed, this, &GeneralTab::availablePluginSelected);
    availPluginsLayout->addWidget(availPluginsTbl);

    downloadPlugin = new QPushButton(this);
    connect(downloadPlugin, &QPushButton::pressed, this, &GeneralTab::downloadPluginClicked);
    downloadPlugin->setEnabled(false);
    availPluginsLayout->addWidget(downloadPlugin);

    connect(this, &GeneralTab::installPlugin, updater, &Updater::checkUpdates);
    connect(this, &GeneralTab::uninstallPlugin, updater, &Updater::uninstallPlugin);

    pluginDesc = new QTextBrowser(this);
    pluginDesc->setReadOnly(true);
    pluginDesc->setOpenExternalLinks(true);
    layout->addRow(pluginDesc);

    QEvent langEvent(QEvent::LanguageChange);
    this->changeEvent(&langEvent);
}

void GeneralTab::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        langLbl->setText(tr("Language:"));
        fullscreenChk->setText(tr("Show main window on full-screen"));
        autostartChk->setText(tr("Run program on startup"));
        installedPluginsLbl->setText(tr("Installed plugins:"));
        availPluginsLbl->setText(tr("Available plugins:"));
        moveUpBtn->setText(tr("Move up"));
        moveDownBtn->setText(tr("Move down"));
        downloadPlugin->setText(tr("Download and install")); // TODO

        pluginsModel->refresh();
        this->installedPluginSelected(pluginsModel->index(pluginsTbl->currentIndex().row(), 1));
    }
}

void GeneralTab::moveUpBtnClicked()
{
    int currRow = pluginsTbl->currentIndex().row();
    if(currRow < 1) return;

    pluginsModel->swapEntries(currRow, currRow-1);
    QModelIndex idx = pluginsModel->index(currRow-1, 0, QModelIndex());
    pluginsTbl->setCurrentIndex(idx);
}

void GeneralTab::moveDownBtnClicked()
{
    int currRow = pluginsTbl->currentIndex().row();
    if(currRow < 0 || currRow >= pluginsModel->rowCount(QModelIndex())-1) return;

    pluginsModel->swapEntries(currRow, currRow+1);
    QModelIndex idx = pluginsModel->index(currRow+1, 0, QModelIndex());
    pluginsTbl->setCurrentIndex(idx);
}

void GeneralTab::downloadPluginClicked()
{
//    int currRow = availPluginsTbl->currentIndex().row();
//    if(currRow < 0 || currRow >= availPluginsModel->rowCount(QModelIndex())) return;

#ifdef Q_OS_WIN
    updater->setInstallPlugin(pluginToChange);

    connect(updater, &Updater::newUpdates, this, [this](UpdateInfoArray info) {
        disconnect(updater, &Updater::newUpdates, this, 0);
        UpdateDialog *dlg = new UpdateDialog(&info, updater);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->exec();
    });

    if(pluginToChange[0] == '+')
    {
        emit installPlugin();
    }
    else if(pluginToChange[0] == '-')
    {
        emit uninstallPlugin();
    }
#endif

#ifdef Q_OS_LINUX
    QString url = availPluginsModel->getPluginInfo(currRow).url;
    // TODO
#endif
}

void GeneralTab::installedPluginSelected(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    if(index.column() == 0)
    {
        pluginsModel->toggleChecked(index.row());
    }

    QString text = "<b>" + pluginsModel->getPluginInfo(index.row()).name + "</b>";
    text += " <b>" + pluginsModel->getPluginInfo(index.row()).version + "</b>";
    text += "<br/><br/>" + pluginsModel->getPluginInfo(index.row()).desc;

    pluginDesc->setText(text);

    downloadPlugin->setText(tr("Uninstall plugin"));
    pluginToChange = "-" + pluginsModel->getPluginInfo(index.row()).id;
    downloadPlugin->setEnabled(true);

    availPluginsTbl->clearSelection();
}

void GeneralTab::availablePluginSelected(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    QString text = "<b>" + availPluginsModel->getPluginInfo(index.row()).name + "</b>";
    text += " <b>" + availPluginsModel->getPluginInfo(index.row()).version + "</b>";
    text += "<br/><br/>" + availPluginsModel->getPluginInfo(index.row()).desc;

    pluginDesc->setText(text);

    downloadPlugin->setText(tr("Download and install plugin"));
    pluginToChange = "+" + availPluginsModel->getPluginInfo(index.row()).id;
    downloadPlugin->setEnabled(true);

    pluginsTbl->clearSelection();
}

void GeneralTab::loadSettings()
{
    QString lang = settings->value("lang", "").toString();
    if(lang.isEmpty())
    {
        lang = QLocale::system().name().left(2);
    }

    currLang = "en";
    int idx = 0;
    for(int i=0; i<langBox->count(); i++)
    {
        if(langBox->itemData(i).toString() == lang)
        {
            currLang = lang;
            idx = i;
            break;
        }
    }
    langBox->setCurrentIndex(idx);

    fullscreenChk->setChecked(settings->value("fullscreen", false).toBool());
    autostartChk->setChecked(autostart->value("Edah", 0) != 0);

    QStringList pluginsId = pluginsModel->load();
    availPluginsModel->refresh(pluginsId);
}

void GeneralTab::writeSettings()
{
    settings->setValue("lang", langBox->currentData());
    settings->setValue("fullscreen", fullscreenChk->isChecked());

    if(autostartChk->isChecked() && autostart->value("Edah", 0) == 0)
        autostart->setValue("Edah", QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/edah.exe"));
    else if(!autostartChk->isChecked() && autostart->value("Edah", 0) != 0)
        autostart->remove("Edah");

    QVector<PluginCfgEntry> cfg;

    for(int i=0; i<pluginsModel->rowCount(QModelIndex()); i++)
    {
        PluginInfo pi = pluginsModel->getPluginInfo(i);

        PluginCfgEntry entry;
        entry.id = pi.id;
        entry.enabled = pi.enabled;
        cfg.push_back(entry);
    }

    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);
    stream << cfg;
    settings->setValue("plugins", arr);
}

////////////////////////
/// PluginTableModel ///
////////////////////////

QStringList PluginTableModel::load()
{
    emit layoutAboutToBeChanged();

    QStringList pluginsId;
    plugins.clear();

    QVector<PluginCfgEntry> cfg;
    QByteArray arr = settings->value("plugins").toByteArray();
    QDataStream stream(arr);
    stream >> cfg;

    for(int i=0; i<cfg.size(); i++)
    {
        QFile file(utils->getDataDir()+"/plugins/"+cfg[i].id+"/info.json");

        if(!file.exists())
        {
            continue;
        }

        PluginInfo pi = this->loadFromFile(file);
        pi.enabled = cfg[i].enabled;

        plugins.push_back(pi);
        pluginsId.push_back(pi.id);
    }

    QStringList pluginsList = QDir(utils->getDataDir()+"/plugins").entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for(int i=0; i<pluginsList.size(); i++)
    {
        QString pluginDir = pluginsList[i];

        if(pluginsId.contains(pluginDir))
        {
            continue;
        }

        QFile file(utils->getDataDir()+"/plugins/"+pluginDir+"/info.json");

        PluginInfo pi = this->loadFromFile(file);
        pi.enabled = false;

        plugins.push_back(pi);
        pluginsId.push_back(pluginDir);
    }

    emit layoutChanged();
    emit dataChanged(createIndex(0, 0), createIndex(plugins.size()-1, 2));

    return pluginsId;
}

PluginInfo PluginTableModel::loadFromFile(QFile &file)
{
    PluginInfo pi;

    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();

    pi.id = info.value("id").toString();
    pi.version = info.value("version").toString();

    pi.name = MultilangString::fromJson(info.value("name").toObject());
    pi.desc = MultilangString::fromJson(info.value("desc").toObject());

    return pi;
}

void PluginTableModel::refresh()
{
    emit dataChanged(createIndex(0, 0), createIndex(plugins.size()-1, 2));
}

int PluginTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return plugins.size();
}

int PluginTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 3;
}

QVariant PluginTableModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(index.row() >= plugins.size()) return QVariant();

        switch(index.column())
        {
        case 1: return plugins[index.row()].name.toString();
        case 2:
        {
            QTextDocument text;
            text.setHtml(plugins[index.row()].desc.toString());
            return text.toPlainText();
        }
        }
    }
    else if(role == Qt::CheckStateRole)
    {
        if(index.column() == 0)
        {
            return plugins[index.row()].enabled ? Qt::Checked : Qt::Unchecked;
        }
    }

    return QVariant();
}

PluginInfo PluginTableModel::getPluginInfo(int i)
{
    if(i<0 || i>=plugins.size())
    {
        return PluginInfo();
    }

    return plugins[i];
}

void PluginTableModel::toggleChecked(int i)
{
    plugins[i].enabled = !plugins[i].enabled;

    emit dataChanged(createIndex(i, 0), createIndex(i, 2));
}

void PluginTableModel::swapEntries(int pos1, int pos2)
{
    qSwap(plugins[pos1], plugins[pos2]);
}

QStringList PluginTableModel::getIds()
{
    QStringList ret;

    for(int i=0; i<plugins.size(); i++)
    {
        ret.append(plugins[i].id);
    }

    return ret;
}

/////////////////////////////
/// AvailPluginTableModel ///
/////////////////////////////

AvailPluginTableModel::AvailPluginTableModel()
{
    manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/get_plugins.php"));
    reply = manager->get(url);

    connect(reply, &QNetworkReply::finished, this, [this](){
        json = QJsonDocument::fromJson(reply->readAll()).object();
        this->refresh(QStringList());
    });
}

AvailPluginTableModel::~AvailPluginTableModel()
{
    delete manager;
}

void AvailPluginTableModel::refresh(QStringList skipPlugins)
{
    static QStringList skip;

    if(skipPlugins.size() > 0)
        skip = skipPlugins;

    plugins.clear();

    emit layoutAboutToBeChanged();

    for(int i=0; i<json.size(); i++)
    {
        QString id = json.keys()[i];

        if(!skip.contains(id))
        {
            PluginInfo pi;

            pi.id = id;
            pi.version = json[id].toObject().value("version").toString();
#ifdef Q_OS_LINUX
            pi.url = json[id].toObject().value("deb_url").toString();
#endif

            pi.name = MultilangString::fromJson(json[id].toObject()["name"].toObject());
            pi.desc = MultilangString::fromJson(json[id].toObject()["desc"].toObject());

            plugins.push_back(pi);
        }
    }

    emit layoutChanged();
    emit dataChanged(createIndex(0, 0), createIndex(plugins.size()-1, 2));
}

int AvailPluginTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return plugins.size();
}

int AvailPluginTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 2;
}

QVariant AvailPluginTableModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(index.row() >= plugins.size()) return QVariant();

        switch(index.column())
        {
        case 0: return plugins[index.row()].name.toString();
        case 1:
        {
            QTextDocument text;
            text.setHtml(plugins[index.row()].desc.toString());
            return text.toPlainText();
        }
        }
    }

    return QVariant();
}

PluginInfo AvailPluginTableModel::getPluginInfo(int i)
{
    if(i<0 || i>=plugins.size())
    {
        return PluginInfo();
    }

    return plugins[i];
}
