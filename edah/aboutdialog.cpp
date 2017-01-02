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

#include "aboutdialog.h"

#include <libedah/utils.h>

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

AboutDialog::AboutDialog()
{
    this->setWindowTitle(tr("About"));

    QGridLayout *layout = new QGridLayout;
    this->setLayout(layout);

    QLabel *icon = new QLabel(this);
    icon->setPixmap(QIcon(":/img/icon.svg").pixmap(QSize(128, 128)));
    icon->setFixedSize(128, 128);
    icon->setScaledContents(true);
    layout->addWidget(icon, 0, 0, 2, 1, Qt::AlignTop);

    QLabel *text = new QLabel(this);
    text->setText(QString("<b>Edah %1</b><br/>").arg(utils->getAppVersion()) +
                  "<br/>"
                  + tr("Edah<br/>"));
    text->setWordWrap(true);
    layout->addWidget(text, 0, 1);

    QLabel *text2 = new QLabel(this);
    text2->setText("<br/>"
                   "This software contains:<br/>"
                   "&#8226; <a href=\"http://www.typicons.com/\">Typicons</a> by Stephen Hutchings licensed under <a href=\"https://creativecommons.org/licenses/by-sa/3.0/\">CC BY-SA</a><br/>"
                   "&#8226; <a href=\"https://www.openssl.org/\">OpenSSL</a> by The OpenSSL Project licensed under <a href=\"https://raw.githubusercontent.com/openssl/openssl/master/LICENSE\">OpenSSL License</a><br/>"
                   "&#8226; <a href=\"https://github.com/qtproject/qt-solutions/tree/master/qtsingleapplication\">QtSingleApplication</a> by Digia Plc and/or its subsidiary(-ies) licensed under BSD license<br/>"
                   "&#8226; <a href=\"http://www.google.com/fonts/specimen/Open+Sans\">Open Sans</a> by Steve Matteson licensed under <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License 2.0</a><br/>"
                   "<br/>");
    text2->setOpenExternalLinks(true);
    text2->setWordWrap(false);
    layout->addWidget(text2, 1, 1);

    QLabel *text3 = new QLabel(this);
    text3->setText("This application uploads to update server data such as unique device ID, "
                   "version of your operating system and IP address.<br/><br/>"
                   "Copyright (C) 2016-2017 Łukasz Matczak &lt;<a href=\"mailto:luk.matczak@gmail.com\">luk.matczak@gmail.com</a>&gt;<br/>"
                   "This program comes with ABSOLUTELY NO WARRANTY. This is "
                   "free software, and you are welcome to redistribute it under "
                   "certain conditions; ");
#ifdef Q_OS_WIN
    text3->setText(text3->text() +
                   "<a href=\"LICENSE.txt\">click here</a> for details.");
#endif
    text3->setOpenExternalLinks(true);
    text3->setWordWrap(true);
    layout->addWidget(text3, 2, 1);

    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeBtn = btns->button(QDialogButtonBox::Close);
    closeBtn->setText(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &AboutDialog::close);
    layout->addWidget(btns, 3, 0, 1, 2);
}
