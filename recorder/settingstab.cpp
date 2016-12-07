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

#include <QFormLayout>
#include <QPushButton>
#include <QApplication>
#include <QFileDialog>
#include <QDateTime>

#include <bass.h>

SettingsTab::SettingsTab(IPlugin *parent) : QWidget(0), plugin(parent)
{
    QFormLayout *layout = new QFormLayout;
    this->setLayout(layout);

    QHBoxLayout *dirLayout = new QHBoxLayout;

    recDevBox = new QComboBox(this);
    recDevBox->setEditable(true);
    recDevBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    BASS_DEVICEINFO info;
    for (int i=0; BASS_RecordGetDeviceInfo(i, &info); i++)
        recDevBox->addItem(info.name);

    layout->addRow(tr("Audio device: "), recDevBox);

    recsDir = new QLineEdit(this);
    dirLayout->addWidget(recsDir);

    QPushButton *recsDirBtn = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DirIcon), "", this);
    recsDirBtn->setToolTip(tr("Select directory"));
    connect(recsDirBtn, &QPushButton::clicked, this, &SettingsTab::recsDirBtn_clicked);
    dirLayout->addWidget(recsDirBtn);

    layout->addRow(tr("Recordings directory: "), dirLayout);

    channels = new QComboBox(this);
    channels->addItem(tr("1 (Mono)"), 1);
    channels->addItem(tr("2 (Stereo)"), 2);
    layout->addRow(tr("Number of channels: "), channels);

    bitrate = new QComboBox(this);
    QList<int> values;
    values << 32 << 40 << 48 << 56 << 64 << 80 << 96 << 112 << 128 << 160 << 192 << 224 << 256 << 320;
    for(int i=0; i<values.size(); i++)
        bitrate->addItem(QString("%1 kbit/s").arg(values[i]), values[i]);

    layout->addRow(tr("Bitrate: "), bitrate);

    sampleRate = new QComboBox(this);
    values.clear();
    values << 32000 << 44100 << 48000;
    for(int i=0; i<values.size(); i++)
        sampleRate->addItem(QString("%1 Hz").arg(values[i]), values[i]);
    layout->addRow(tr("Sample rate: "), sampleRate);

    QVBoxLayout *filenameFmtLayout = new QVBoxLayout;

    filenameFmt = new QComboBox(this);
    filenameFmt->setEditable(true);
    connect(filenameFmt, &QComboBox::currentTextChanged, this, &SettingsTab::filenameFmt_changed);
    filenameFmtLayout->addWidget(filenameFmt);

    filenameExample = new QLabel(this);
    filenameExample->setWordWrap(true);
    filenameFmtLayout->addWidget(filenameExample);

    QLabel *filenameHelp = new QLabel(this);
    filenameHelp->setTextFormat(Qt::RichText);
    filenameHelp->setText(tr("Available tags:<br/>"
                             "<b>%d%</b> - the day (1 to 31)<br/>"
                             "<b>%dd%</b> - the day with a leading zero (01 to 31)<br/>"
                             "<b>%ddd%</b> - the abbreviated day name ('Mon' to 'Sun')<br/>"
                             "<b>%dddd%</b> - the long day name ('Monday' to 'Sunday')<br/>"
                             "<b>%M%</b> - the month (1-12)<br/>"
                             "<b>%MM%</b> - the month with a leading zero (01-12)<br/>"
                             "<b>%MMM%</b> - the abbreviated month name ('Jan' to 'Dec')<br/>"
                             "<b>%MMMM%</b> - the long month name ('January' to 'December')<br/>"
                             "<b>%yy%</b> - the year as two digit number (00-99)<br/>"
                             "<b>%yyyy%</b> - the year as four digit number<br/><br/>"
                             "<b>%h%</b> - the hour (0 to 23 or 1 to 12 if AM/PM display)<br/>"
                             "<b>%hh%</b> - the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)<br/>"
                             "<b>%H%</b> - the hour (0 to 23, even with AM/PM display)<br/>"
                             "<b>%HH%</b> - the hour with a leading zero (00 to 23, even with AM/PM display)<br/>"
                             "<b>%m%</b> - the minute (0 to 59)<br/>"
                             "<b>%mm%</b> - the minute with a leading zero (00 to 59)<br/>"
                             "<b>%s%</b> - the second (0 to 59)<br/>"
                             "<b>%ss%</b> - the second with a leading zero (00 to 59)<br/>"
                             "<b>%AP%</b> - 'AM' or 'PM'<br/>"
                             "<b>%ap%</b> - 'am' or 'pm'<br/><br/>"
                             "<b>%n%</b> - filename entered by user"));
    filenameFmtLayout->addWidget(filenameHelp);

    layout->addRow(tr("Default filename: "), filenameFmtLayout);
}

void SettingsTab::loadSettings()
{
    settings->beginGroup(plugin->getPluginId());

    recDevBox->setCurrentText(settings->value("device", "").toString());
    recsDir->setText(settings->value("recsDir", "").toString());
    channels->setCurrentIndex(settings->value("channels", 1).toInt()-1);
    bitrate->setCurrentText(QString("%1 kbit/s").arg(settings->value("bitrate", 64).toInt()));
    sampleRate->setCurrentText(QString("%1 Hz").arg(settings->value("sampleRate", 44100).toInt()));

    QString fmt = settings->value("filenameFormat", "%n% %yyyy%-%MM%-%dd% %hh%.%mm%").toString();
    QStringList fmts;
    fmts << "%n% %yyyy%-%MM%-%dd% %hh%.%mm%"
         << "%yyyy%-%MM%-%dd% %n%"
         << "%n%";

    if(!fmts.contains(fmt))
    {
        fmts.insert(0, fmt);
    }

    filenameFmt->clear();
    filenameFmt->addItems(fmts);
    filenameFmt->setCurrentIndex(fmts.indexOf(fmt));

    settings->endGroup();
}

void SettingsTab::writeSettings()
{
    settings->beginGroup(plugin->getPluginId());

    settings->setValue("device", recDevBox->currentText());
    settings->setValue("recsDir", recsDir->text());
    settings->setValue("channels", channels->currentData().toInt());
    settings->setValue("bitrate", bitrate->currentData().toInt());
    settings->setValue("sampleRate", sampleRate->currentData().toInt());
    settings->setValue("filenameFormat", filenameFmt->currentText());

    settings->endGroup();
}

void SettingsTab::recsDirBtn_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, QString(), recsDir->text());
    if(!path.isEmpty())
    {
        recsDir->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsTab::filenameFmt_changed(const QString &text)
{
    filenameExample->setText(tr("Example: ") + utils->parseFilename(text, tr("Recording"), QDateTime::currentDateTime()));
}
