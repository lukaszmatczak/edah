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

    recDevBox = new QComboBox(this);
    recDevBox->setEditable(true);
    recDevBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    BASS_DEVICEINFO info;
    for (int i=0; BASS_RecordGetDeviceInfo(i, &info); i++)
        recDevBox->addItem(info.name);

    layout->addRow(tr("Audio device: "), recDevBox);

    shoutcastBox = new QGroupBox(tr("SHOUTcast"), this);
    shoutcastBox->setCheckable(true);
    QFormLayout *shoutcastBoxLayout = new QFormLayout;
    shoutcastBox->setLayout(shoutcastBoxLayout);

    sc_version = new QComboBox(this);
    sc_version->addItem("SHOUTcast v1", 1);
    sc_version->addItem("SHOUTcast v2", 2);
    connect(sc_version, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsTab::sc_versionChanged);
    shoutcastBoxLayout->addRow(tr("Version: "), sc_version);

    sc_url = new QLineEdit(this);
    shoutcastBoxLayout->addRow(tr("Server URL: "), sc_url);

    sc_port = new QLineEdit(this);
    sc_port->setValidator(new QIntValidator(0, 65535));
    shoutcastBoxLayout->addRow(tr("Server port: "), sc_port);

    sc_streamid = new QLineEdit(this);
    sc_streamid->setValidator(new QIntValidator(1, 2147483647));
    shoutcastBoxLayout->addRow(tr("Stream ID: "), sc_streamid);

    sc_username = new QLineEdit(this);
    shoutcastBoxLayout->addRow(tr("Username: "), sc_username);

    sc_password = new QLineEdit(this);
    sc_password->setEchoMode(QLineEdit::Password);
    shoutcastBoxLayout->addRow(tr("Password: "), sc_password);

    sc_channels = new QComboBox(this);
    sc_channels->addItem(tr("1 (Mono)"), 1);
    sc_channels->addItem(tr("2 (Stereo)"), 2);
    shoutcastBoxLayout->addRow(tr("Number of channels: "), sc_channels);

    sc_bitrate = new QComboBox(this);
    QList<int> values;
    values << 32 << 40 << 48 << 56 << 64 << 80 << 96 << 112 << 128 << 160 << 192 << 224 << 256 << 320;
    for(int i=0; i<values.size(); i++)
        sc_bitrate->addItem(QString("%1 kbit/s").arg(values[i]), values[i]);

    shoutcastBoxLayout->addRow(tr("Bitrate: "), sc_bitrate);

    sc_sampleRate = new QComboBox(this);
    values.clear();
    values << 32000 << 44100 << 48000;
    for(int i=0; i<values.size(); i++)
        sc_sampleRate->addItem(QString("%1 Hz").arg(values[i]), values[i]);
    shoutcastBoxLayout->addRow(tr("Sample rate: "), sc_sampleRate);

    layout->addRow(shoutcastBox);


    voipBox = new QGroupBox(tr("VoIP (ipfon.pl)"), this);
    voipBox->setCheckable(true);
    QFormLayout *voipBoxLayout = new QFormLayout;
    voipBox->setLayout(voipBoxLayout);

    QHBoxLayout *voip_usernameLayout = new QHBoxLayout;
    QLabel *voip_usernamePrefix = new QLabel("sip:", this);
    voip_usernameLayout->addWidget(voip_usernamePrefix);
    voip_username = new QLineEdit(this);
    voip_usernameLayout->addWidget(voip_username);
    QLabel *voip_usernameSuffix = new QLabel("@sip.ipfon.pl", this);
    voip_usernameLayout->addWidget(voip_usernameSuffix);
    voipBoxLayout->addRow(tr("Username: "), voip_usernameLayout);

    voip_password = new QLineEdit(this);
    voip_password->setEchoMode(QLineEdit::Password);
    voipBoxLayout->addRow(tr("Password: "), voip_password);

    voip_port = new QComboBox(this);
    voip_port->setEditable(true);
    voip_port->setValidator(new QIntValidator(0, 65535));
    voipBoxLayout->addRow(tr("SIP port: "), voip_port);

    voip_number = new QLineEdit(this);
    voip_number->setValidator(new QIntValidator(0, 999999999));
    voipBoxLayout->addRow(tr("Phone number: "), voip_number);

    voip_pin = new QLineEdit(this);
    voip_pin->setValidator(new QIntValidator(0, 9999));
    voip_pin->setEchoMode(QLineEdit::Password);
    voipBoxLayout->addRow(tr("PIN: "), voip_pin);

    voip_playDev = new QComboBox(this);
    voip_playDev->setEditable(true);
    voip_playDev->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    voip_playDev->addItem(tr("Mute"));

    for (int i=1; BASS_GetDeviceInfo(i, &info); i++)
        voip_playDev->addItem(info.name);

    voipBoxLayout->addRow(tr("Play device: "), voip_playDev);

    layout->addRow(voipBox);
}

void SettingsTab::sc_versionChanged(int index)
{
    sc_streamid->setEnabled(index == 1);
    sc_username->setEnabled(index == 1);
}

void SettingsTab::loadSettings()
{
    settings->beginGroup(plugin->getPluginId());

    recDevBox->setCurrentText(settings->value("device", "").toString());

    shoutcastBox->setChecked(settings->value("shoutcast", false).toBool());
    sc_version->setCurrentIndex(settings->value("sc_version", 1).toInt());
    sc_url->setText(settings->value("sc_url").toString());
    sc_port->setText(settings->value("sc_port").toString());
    sc_streamid->setText(settings->value("sc_streamid").toString());
    sc_username->setText(settings->value("sc_username").toString());
    sc_password->setText(QByteArray::fromBase64(settings->value("sc_password").toByteArray()));
    sc_channels->setCurrentIndex(settings->value("sc_channels", 1).toInt()-1);
    sc_bitrate->setCurrentText(QString("%1 kbit/s").arg(settings->value("sc_bitrate", 64).toInt()));
    sc_sampleRate->setCurrentText(QString("%1 Hz").arg(settings->value("sc_sampleRate", 44100).toInt()));
    this->sc_versionChanged(sc_version->currentIndex()); // force interface refresh

    voipBox->setChecked(settings->value("voip", false).toBool());
    voip_username->setText(settings->value("voip_username").toString());
    voip_password->setText(QByteArray::fromBase64(settings->value("voip_password").toByteArray()));
    voip_number->setText(settings->value("voip_number").toString());
    voip_pin->setText(QByteArray::fromBase64(settings->value("voip_pin").toByteArray()));

    QString vPort = settings->value("voip_port", "5060").toString();
    QStringList ports;
    ports << "5060" << "6060";

    if(!ports.contains(vPort))
    {
        ports.insert(0, vPort);
    }

    voip_port->clear();
    voip_port->addItems(ports);
    voip_port->setCurrentIndex(ports.indexOf(vPort));

    QString playDev = settings->value("voip_playDev", "Mute").toString();
    if(playDev == "Mute")
        voip_playDev->setCurrentIndex(0);
    else
        voip_playDev->setCurrentText(playDev);

    settings->endGroup();
}

void SettingsTab::writeSettings()
{
    settings->beginGroup(plugin->getPluginId());

    settings->setValue("device", recDevBox->currentText());

    settings->setValue("shoutcast", shoutcastBox->isChecked());
    settings->setValue("sc_version", sc_version->currentIndex());
    settings->setValue("sc_url", sc_url->text());
    settings->setValue("sc_port", sc_port->text());
    settings->setValue("sc_streamid", sc_streamid->text());
    settings->setValue("sc_username", sc_username->text());
    settings->setValue("sc_password", QString::fromUtf8(sc_password->text().toUtf8().toBase64()));
    settings->setValue("sc_channels", sc_channels->currentData().toInt());
    settings->setValue("sc_bitrate", sc_bitrate->currentData().toInt());
    settings->setValue("sc_sampleRate", sc_sampleRate->currentData().toInt());

    settings->setValue("voip", voipBox->isChecked());
    settings->setValue("voip_username", voip_username->text());
    settings->setValue("voip_password", QString::fromUtf8(voip_password->text().toUtf8().toBase64()));
    settings->setValue("voip_port", voip_port->currentText());
    settings->setValue("voip_number", voip_number->text());
    settings->setValue("voip_pin", QString::fromUtf8(voip_pin->text().toUtf8().toBase64()));

    QString playDev = "Mute";
    if(voip_playDev->currentIndex() > 0)
        playDev = voip_playDev->currentText();
    settings->setValue("voip_playDev", playDev);

    settings->endGroup();
}
