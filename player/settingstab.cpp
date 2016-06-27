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

#include "settingstab.h"

#include <libedah/utils.h>

#include <bass.h>

#include <QFormLayout>
#include <QPushButton>
#include <QApplication>
#include <QStyle>
#include <QFileDialog>
#include <QLabel>

SettingsTab::SettingsTab(IPlugin *parent) : QWidget(0), plugin(parent)
{
    QFormLayout *layout = new QFormLayout;
    this->setLayout(layout);

    QHBoxLayout *songsLayout = new QHBoxLayout;

    playDevBox = new QComboBox(this);
    playDevBox->setEditable(true);
    playDevBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    BASS_DEVICEINFO info;
    for (int i=1; BASS_GetDeviceInfo(i, &info); i++)
        playDevBox->addItem(info.name);

    layout->addRow(tr("Audio device: "), playDevBox);

    songsDir = new QLineEdit(this);
    songsLayout->addWidget(songsDir);

    QPushButton *songsDirBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DirIcon), "", this);
    songsDirBtn->setToolTip(tr("Select directory"));
    connect(songsDirBtn, &QPushButton::clicked, this, &SettingsTab::songsDirBtn_clicked);
    songsLayout->addWidget(songsDirBtn);

    layout->addRow(tr("Songs directory: "), songsLayout);
}

void SettingsTab::songsDirBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, QString(), songsDir->text());
    if(!path.isEmpty())
    {
        songsDir->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTab::loadSettings()
{
    settings->beginGroup(plugin->getPluginId());

    playDevBox->setCurrentText(settings->value("device", "").toString());
    songsDir->setText(settings->value("songsDir", "").toString());
    settings->endGroup();

}

void SettingsTab::writeSettings()
{
    settings->beginGroup(plugin->getPluginId());

    settings->setValue("device", playDevBox->currentText());
    settings->setValue("songsDir", songsDir->text());

    settings->endGroup();
}
