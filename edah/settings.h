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

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QTableView>
#include <QTextBrowser>
#include <QFile>
#include <QDialogButtonBox>
#include <QLabel>

class PluginTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    struct PluginInfo
    {
        bool enabled;
        QString id;
        QString name;
        QString desc;
        QString version;
    };

    void load(QString lang);
    PluginInfo loadFromFile(QFile &file, const QString &lang);
    void retranslate(const QString &lang);
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    void swapEntries(int pos1, int pos2);

    PluginInfo getPluginInfo(int i);
    void toggleChecked(int i);

private:
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
    QLabel *installedPluginsLbl;
    QLabel *availPluginsLbl;
    QPushButton *moveUpBtn;
    QPushButton *moveDownBtn;

    QTextBrowser *pluginDesc;

    QString currLang;

private slots:
    void installedPluginSelected(const QModelIndex &index);
    void moveUpBtnClicked();
    void moveDownBtnClicked();
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
