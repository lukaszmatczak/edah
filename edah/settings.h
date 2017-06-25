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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "mainwindow.h"

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QFile>
#include <QDialogButtonBox>
#include <QLabel>

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
    QCheckBox *autostartChk;
    QCheckBox *keepScreenChk;

    QSettings *autostart;

    QString currLang;
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
