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

#include <QDebug>

////////////////
/// Settings ///
////////////////

Settings::Settings(QVector<Plugin> *plugins) : plugins(plugins)
{
    this->setLayout(new QVBoxLayout);

    tabs = new QTabWidget(this);

    generalTab = new GeneralTab();
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

    autostart = new QSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    autostartChk = new QCheckBox(this);
    layout->addRow(autostartChk);

    keepScreenChk = new QCheckBox(this);
    layout->addRow(keepScreenChk);

    QHBoxLayout *pluginsLayout = new QHBoxLayout;
    layout->addRow(pluginsLayout);

    QVBoxLayout *installedPluginsLayout = new QVBoxLayout;
    pluginsLayout->addLayout(installedPluginsLayout);

    QEvent langEvent(QEvent::LanguageChange);
    this->changeEvent(&langEvent);
}

void GeneralTab::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        langLbl->setText(tr("Language:"));
        autostartChk->setText(tr("Run program on startup"));
        keepScreenChk->setText(tr("Prevent screensaver"));
    }
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

    autostartChk->setChecked(autostart->value("Edah", 0) != 0);
    keepScreenChk->setChecked(settings->value("keepScreen", false).toBool());
}

void GeneralTab::writeSettings()
{
    settings->setValue("lang", langBox->currentData());
    settings->setValue("keepScreen", keepScreenChk->isChecked());

    if(autostartChk->isChecked() && autostart->value("Edah", 0) == 0)
        autostart->setValue("Edah", QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/edah.exe"));
    else if(!autostartChk->isChecked() && autostart->value("Edah", 0) != 0)
        autostart->remove("Edah");
}
