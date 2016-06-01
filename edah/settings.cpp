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
#include "database.h"
#include "osutils.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFormLayout>
#include <QStandardItem>
#include <QLabel>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>

#include <QSqlQuery>

#include <QDebug>

Settings::Settings()
{
    this->setWindowTitle(tr("Settings"));

    this->setLayout(new QVBoxLayout);

    QTabWidget *tabs = new QTabWidget(this);

    tab.push_back(new GeneralTab);
    tabs->addTab(tab.back(), tr("General"));
    this->layout()->addWidget(tabs);

    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel |
                                                  QDialogButtonBox::Apply);
    btns->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    btns->button(QDialogButtonBox::Apply)->setText(tr("Apply"));

    connect(btns, &QDialogButtonBox::accepted, this, [this]() {
        this->writeSettings();
        this->close();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &Settings::close);
    connect(btns->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &Settings::writeSettings);

    this->layout()->addWidget(btns);

    foreach(Tab *t, tab)
    {
        t->loadSettings();
    }
}

void Settings::writeSettings()
{
    foreach(Tab *t, tab)
    {
        t->writeSettings();
    }
}

GeneralTab::GeneralTab()
{
    QFormLayout *layout = new QFormLayout;
    this->setLayout(layout);

    langBox = new QComboBox(this);
    langBox->addItem("English", "en");
    langBox->addItem("polski", "pl");

    layout->addRow(tr("Language:"), langBox);

    fullscreenChk = new QCheckBox(tr("Show main window on full-screen"), this);
    layout->addRow(fullscreenChk);

    QHBoxLayout *pluginsLayout = new QHBoxLayout;
    layout->addRow(pluginsLayout);

    QVBoxLayout *installedPluginsLayout = new QVBoxLayout;
    pluginsLayout->addLayout(installedPluginsLayout);

    QLabel *installedPluginsLbl = new QLabel(tr("Installed plugins:"), this);
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

    QLabel *availPluginsLbl = new QLabel(tr("Available plugins:"), this);
    availPluginsLayout->addWidget(availPluginsLbl);

    pluginDesc = new QTextEdit(this);
    pluginDesc->setReadOnly(true);
    layout->addRow(pluginDesc);
}

void GeneralTab::installedPluginSelected(const QModelIndex &index)
{
    if(index.column() == 0)
    {
        pluginsModel->toggleChecked(index.row());
    }

    QString text = "<b>" + pluginsModel->getPluginInfo(index.row()).name + "</b>";
    text += "<br/><br/>" + pluginsModel->getPluginInfo(index.row()).desc;
    text += "<br/><br/>" + tr("Installed version: ") + pluginsModel->getPluginInfo(index.row()).version;

    pluginDesc->setText(text);
}

void GeneralTab::loadSettings()
{
    QString lang = db->getValue("lang", "").toString();
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

    fullscreenChk->setChecked(db->getValue("fullscreen", false).toBool());

    pluginsModel->load(currLang);
}

void GeneralTab::writeSettings()
{
    db->setValue("lang", langBox->currentData());
    db->setValue("fullscreen", fullscreenChk->isChecked());

    db->db.exec("DELETE FROM `plugins`");

    for(int i=0; i<pluginsModel->rowCount(QModelIndex()); i++)
    {
        PluginTableModel::PluginInfo pi = pluginsModel->getPluginInfo(i);

        QSqlQuery q(db->db);
        q.prepare("INSERT INTO `plugins` VALUES(null, :plugin_id, :order, :enabled)");
        q.bindValue(":plugin_id", pi.id);
        q.bindValue(":order", i);
        q.bindValue(":enabled", pi.enabled);
        q.exec();
    }
}

void PluginTableModel::load(QString lang)
{
    emit layoutAboutToBeChanged();

    plugins.clear();

    QStringList pluginsList = QDir(utils->getDataDir()+"/plugins").entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for(int i=0; i<pluginsList.size(); i++)
    {
        QString pluginDir = pluginsList[i];
        QFile file(utils->getDataDir()+"/plugins/"+pluginDir+"/info.json");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QJsonObject info = QJsonDocument::fromJson(file.readAll()).object();
        QJsonObject infoLang = info.value(lang).toObject();
        if(infoLang.isEmpty())
        {
            infoLang = info.value("en").toObject();
        }

        QSqlQuery q(db->db);
        q.prepare("SELECT `enabled` FROM `plugins` WHERE `plugin_id`=:plugin_id");
        q.bindValue(":plugin_id", info.value("id"));
        q.exec();
        q.first();

        PluginInfo pi;
        pi.enabled = q.value(0).toBool();
        pi.id = info.value("id").toString();
        pi.name = infoLang.value("name").toString();
        pi.desc = infoLang.value("desc").toString();
        pi.version = info.value("version").toString();

        plugins.push_back(pi);
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

PluginTableModel::PluginInfo &PluginTableModel::getPluginInfo(int i)
{
    return plugins[i];
}

void PluginTableModel::toggleChecked(int i)
{
    plugins[i].enabled = !plugins[i].enabled;

    emit dataChanged(createIndex(i, 0), createIndex(i, 2));
}
