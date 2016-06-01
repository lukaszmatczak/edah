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

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QTableView>
#include <QTextEdit>

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
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    PluginInfo &getPluginInfo(int i);
    void toggleChecked(int i);

private:
    QVector<PluginInfo> plugins;

};

class Tab : public QWidget
{
    Q_OBJECT
public:
    virtual void loadSettings() = 0;
    virtual void writeSettings() = 0;
};

class Settings : public QDialog
{
    Q_OBJECT

public:
    Settings();

private:
    QVector<Tab*> tab;

private slots:
    void writeSettings();
};

class GeneralTab : public Tab
{
    Q_OBJECT

public:
    GeneralTab();
    void loadSettings();
    void writeSettings();

private:
    QComboBox *langBox;
    QCheckBox *fullscreenChk;
    QTableView *pluginsTbl;
    PluginTableModel *pluginsModel;

    QTextEdit *pluginDesc;

    QString currLang;

private slots:
    void installedPluginSelected(const QModelIndex &index);
};

#endif // SETTINGS_H
