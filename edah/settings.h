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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "mainwindow.h"

#include <libedah/multilangstring.h>

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QTableView>
#include <QTextBrowser>
#include <QFile>
#include <QDialogButtonBox>
#include <QLabel>
#include <QNetworkAccessManager>

struct PluginInfo
{
    bool enabled;
    QString id;
    MultilangString name;
    MultilangString desc;
    QString version;
#ifdef Q_OS_LINUX
    QString url;
#endif
};

class PluginTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    void load();
    PluginInfo loadFromFile(QFile &file);
    void refresh();
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    void swapEntries(int pos1, int pos2);

    PluginInfo getPluginInfo(int i);
    void toggleChecked(int i);

private:
    QVector<PluginInfo> plugins;

};

class AvailPluginTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    AvailPluginTableModel();
    virtual ~AvailPluginTableModel();
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    PluginInfo getPluginInfo(int i);

private slots:
    void pluginsDownloaded();

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;

    QVector<PluginInfo> plugins;
};

class GeneralTab : public QWidget
{
    Q_OBJECT

public:
    GeneralTab();
    void loadSettings();
    void writeSettings();

protected:
    void changeEvent(QEvent *e);

private:
    QLabel *langLbl;
    QComboBox *langBox;
    QCheckBox *fullscreenChk;

    QTableView *pluginsTbl;
    PluginTableModel *pluginsModel;

    QTableView *availPluginsTbl;
    AvailPluginTableModel *availPluginsModel;

    QLabel *installedPluginsLbl;
    QLabel *availPluginsLbl;
    QPushButton *moveUpBtn;
    QPushButton *moveDownBtn;
    QPushButton *downloadPlugin;

    QTextBrowser *pluginDesc;

    QString currLang;

private slots:
    void installedPluginSelected(const QModelIndex &index);
    void availablePluginSelected(const QModelIndex &index);
    void moveUpBtnClicked();
    void moveDownBtnClicked();
    void downloadPluginClicked();
};

class Settings : public QDialog
{
    Q_OBJECT

public:
    Settings(QVector<Plugin> *plugins);
    virtual ~Settings();

protected:
    void changeEvent(QEvent *e);

private:
    QTabWidget *tabs;
    GeneralTab *generalTab;
    QVector<Plugin> *plugins;
    QDialogButtonBox *dialogBtns;

signals:
    void settingsChanged();

private slots:
    void writeSettings();
};

#endif // SETTINGS_H
