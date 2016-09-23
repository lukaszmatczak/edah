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

#include "updatedialog.h"
#include <QPushButton>
#include <QApplication>
#include <QStyle>

UpdateDialog::UpdateDialog(UpdateInfoArray *info, Updater *updater, QWidget *parent) :
    QDialog(parent), info(info), updater(updater)
{
    layout = new QVBoxLayout(this);
    this->setLayout(layout);

    int install = 0, remove = 0, update = 0;
    for(int i=0; i<info->size(); i++)
    {
        switch(info->at(i).action)
        {
        case UpdateInfo::Install: install++; break;
        case UpdateInfo::Uninstall: remove++; break;
        case UpdateInfo::Update: update++; break;
        }
    }

    label = new QLabel(this);
    QString text;

    if((update > 0) && (install == 0) && (remove == 0))
        text += tr("<b>New version is available!</b><br/><br/>");

    text += tr("Following components will be changed:<br/>");

    for(int i=0; i<info->size(); i++)
    {
        switch(info->at(i).action)
        {
        case UpdateInfo::Install:
            text += tr("&nbsp;&nbsp;&nbsp;\"%1\" (%2) will be installed<br/>")
                    .arg(info->at(i).name)
                    .arg(info->at(i).newVersion);
            break;
        case UpdateInfo::Uninstall:
            text += tr("&nbsp;&nbsp;&nbsp;\"%1\" will be removed<br/>")
                    .arg(info->at(i).name);
            break;
        case UpdateInfo::Update:
            text += tr("&nbsp;&nbsp;&nbsp;\"%1\" will be updated from version %2 to %3<br/>")
                    .arg(info->at(i).name)
                    .arg(info->at(i).oldVersion)
                    .arg(info->at(i).newVersion);
            break;
        }
    }

    label->setText(text);
    layout->addWidget(label);

    sizeLabel = new QLabel(tr("Size of data to download: ... KB"), this);
    layout->addWidget(sizeLabel);

    if((update == 0) && (install == 0))
        sizeLabel->hide();

    QLabel *label2 = new QLabel(tr("Do you want to close Edah and apply these changes?"), this);
    layout->addWidget(label2);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No,
                                     Qt::Horizontal,
                                     this);

    buttonBox->button(QDialogButtonBox::Yes)->setIcon(QApplication::style()->standardIcon(QStyle::SP_VistaShield));
    buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Yes"));
    buttonBox->button(QDialogButtonBox::No)->setText(tr("No"));

    connect(buttonBox, &QDialogButtonBox::accepted, updater, &Updater::prepareUpdate);
    connect(buttonBox, &QDialogButtonBox::rejected, this, [this]() {
        this->updater->setInstallPlugin("");
        this->close();
    });

    layout->addWidget(buttonBox);

    connect(this, &UpdateDialog::checkFiles, updater, &Updater::checkFiles);
    connect(updater, &Updater::filesChecked, this, &UpdateDialog::filesChecked);

    emit checkFiles();
}

UpdateDialog::~UpdateDialog()
{

}

void UpdateDialog::filesChecked(int size)
{
    sizeLabel->setText(tr("Size of data to download: %1 KB").arg(QLocale().toString(size/1024)));
}
