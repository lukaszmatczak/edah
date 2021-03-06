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

#include "updatedialog.h"
#include <QPushButton>
#include <QApplication>
#include <QStyle>

UpdateDialog::UpdateDialog(UpdateInfoArray *info, Updater *updater, QWidget *parent) :
    QDialog(parent), info(info), updater(updater)
{
    layout = new QVBoxLayout(this);
    this->setLayout(layout);

    label = new QLabel(this);
    QString text = tr("<b>New version is available!</b><br/><br/>");

    text += tr("Following components will be updated:<br/>");

    for(int i=0; i<info->size(); i++)
    {
        text += tr("&nbsp;&nbsp;&nbsp;\"%1\" (from %2 to %3)<br/>")
                .arg(info->at(i).name)
                .arg(info->at(i).oldVersion)
                .arg(info->at(i).newVersion);
    }

    label->setText(text);
    layout->addWidget(label);

    sizeLabel = new QLabel(tr("Size of data to download: ... KB"), this);
    layout->addWidget(sizeLabel);

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
