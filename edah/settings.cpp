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

#include "settings.h"

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

#include <QDebug>

////////////////
/// Settings ///
////////////////

Settings::Settings(QVector<Plugin> *plugins) : plugins(plugins)
{
    this->setLayout(new QVBoxLayout);

    tabs = new QTabWidget(this);

    generalTab = new GeneralTab;
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

    QEvent langEvent(QEvent::LanguageChange);
    this->changeEvent(&langEvent);
}

Settings::~Settings()
{
    for(int i=1; i<tabs->count(); i++)
    {
        tabs->widget(i)->setParent(0);
    }
}

void Settings::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        this->setWindowTitle(tr("Settings"));

        tabs->setTabText(0, tr("General"));

        dialogBtns->button(QDialogButtonBox::Ok)->setText(tr("OK"));
        dialogBtns->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
        dialogBtns->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
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

    // TODO: reload tabs
}

//////////////////
/// GeneralTab ///
//////////////////

GeneralTab::GeneralTab()
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
    connect(pluginsTbl, &QTableView::pressed, this, &GeneralTab::installedPluginSelected);
    installedPluginsLayout->addWidget(pluginsTbl);

    QVBoxLayout *availPluginsLayout = new QVBoxLayout;
    pluginsLayout->addLayout(availPluginsLayout);

    availPluginsLbl = new QLabel(this);
    availPluginsLayout->addWidget(availPluginsLbl);

    QHBoxLayout *pluginsBtnsLayout = new QHBoxLayout;
    layout->addRow(pluginsBtnsLayout);

    moveUpBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_ArrowUp), "", this);
    connect(moveUpBtn, &QPushButton::clicked, this, &GeneralTab::moveUpBtnClicked);
    pluginsBtnsLayout->addWidget(moveUpBtn);

    moveDownBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_ArrowDown), "", this);
    connect(moveDownBtn, &QPushButton::clicked, this, &GeneralTab::moveDownBtnClicked);
    pluginsBtnsLayout->addWidget(moveDownBtn);

    pluginDesc = new QTextEdit(this);
    pluginDesc->setReadOnly(true);
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
        installedPluginsLbl->setText(tr("Installed plugins:"));
        availPluginsLbl->setText(tr("Available plugins:"));
        moveUpBtn->setText(tr("Move up"));
        moveDownBtn->setText(tr("Move down"));

        pluginsModel->retranslate(langBox->currentData().toString());
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

void GeneralTab::installedPluginSelected(const QModelIndex &index)
{
    if(index.column() == 0)
    {
        pluginsModel->toggleChecked(index.row());
    }

    QString text = "<b>" + pluginsModel->getPluginInfo(index.row()).name + "</b>";
    text += " <b>" + pluginsModel->getPluginInfo(index.row()).version + "</b>";
    text += "<br/><br/>" + pluginsModel->getPluginInfo(index.row()).desc;

    pluginDesc->setText(text);
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

    pluginsModel->load(currLang);
}

void GeneralTab::writeSettings()
{
    settings->setValue("lang", langBox->currentData());
    settings->setValue("fullscreen", fullscreenChk->isChecked());

    QVector<PluginCfgEntry> cfg;

    for(int i=0; i<pluginsModel->rowCount(QModelIndex()); i++)
    {
        PluginTableModel::PluginInfo pi = pluginsModel->getPluginInfo(i);

        PluginCfgEntry entry;
        entry.id = pi.id;
        entry.enabled = pi.enabled;
        cfg.push_back(entry);
    }

    QFile file(utils->getConfigPath() + "/plugins.cfg", this);
    file.open(QIODevice::WriteOnly);
    QDataStream stream(&file);
    stream << cfg;
}

////////////////////////
/// PluginTableModel ///
////////////////////////

void PluginTableModel::load(QString lang)
{
    emit layoutAboutToBeChanged();

    QVector<QString> pluginsId;
    plugins.clear();

    QVector<PluginCfgEntry> cfg;
    QFile file(utils->getConfigPath() + "/plugins.cfg", this);
    file.open(QIODevice::ReadOnly);
    QDataStream stream(&file);
    stream >> cfg;

    for(int i=0; i<cfg.size(); i++)
    {
        QFile file(utils->getDataDir()+"/plugins/"+cfg[i].id+"/info.json");

        if(!file.exists())
        {
            continue;
        }

        PluginInfo pi = this->loadFromFile(file, lang);
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

        PluginInfo pi = this->loadFromFile(file, lang);
        pi.enabled = false;

        plugins.push_back(pi);
    }

    emit layoutChanged();
    emit dataChanged(createIndex(0, 0), createIndex(plugins.size()-1, 2));
}

PluginTableModel::PluginInfo PluginTableModel::loadFromFile(QFile &file, const QString &lang)
{
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();
    QJsonObject infoLang = info.value(lang).toObject();
    if(infoLang.isEmpty())
    {
        infoLang = info.value("en").toObject();
    }

    PluginInfo pi;
    pi.id = info.value("id").toString();
    pi.name = infoLang.value("name").toString();
    pi.desc = infoLang.value("desc").toString();
    pi.version = info.value("version").toString();

    return pi;
}

void PluginTableModel::retranslate(const QString &lang)
{
    emit layoutAboutToBeChanged();

    for(int i=0; i<plugins.size(); i++)
    {
        QString pluginDir = plugins[i].id;
        QFile file(utils->getDataDir()+"/plugins/"+pluginDir+"/info.json");

        if(!file.exists())
        {
            continue;
        }

        PluginInfo pi = this->loadFromFile(file, lang);

        plugins[i].name = pi.name;
        plugins[i].desc = pi.desc;
    }

    emit layoutChanged();
    emit dataChanged(createIndex(0, 0), createIndex(plugins.size()-1, 2));
}

int PluginTableModel::rowCount(const QModelIndex &parent) const
{
    return plugins.size();
}

int PluginTableModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant PluginTableModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(index.row() >= plugins.size()) return QVariant();

        switch(index.column())
        {
        case 1: return plugins[index.row()].name;
        case 2: return plugins[index.row()].desc;
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

PluginTableModel::PluginInfo PluginTableModel::getPluginInfo(int i)
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
