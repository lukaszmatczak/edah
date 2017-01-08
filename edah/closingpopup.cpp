/*
    Edah
    Copyright (C) 2017  Lukasz Matczak

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

#include "closingpopup.h"

#include <QLabel>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QApplication>
#include <QtSvg/QSvgRenderer>

#include <libedah/logger.h>

#include <Windows.h>

ClosingPopup::ClosingPopup(QWidget *parent) : Popup(parent), parent(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QStackedWidget *stacked = new QStackedWidget(this);
    mainLayout->addWidget(stacked);

    QWidget *first = new QWidget(this);
    QHBoxLayout *layout1 = new QHBoxLayout(first);

    shutdownBtn = new MyPushButton(tr("Shutdown computer"), this);
    shutdownBtn->setObjectName("shutdownBtn");
    shutdownBtn->setIcon(QIcon(":/img/power.svg"));
    connect(shutdownBtn, &MyPushButton::clicked, this, &ClosingPopup::shutdown);
    layout1->addWidget(shutdownBtn);

    closeBtn = new MyPushButton(tr("Close app"), this);
    closeBtn->setObjectName("closeBtn_");
    closeBtn->setIcon(QIcon(":/img/close.svg"));
    connect(closeBtn, &MyPushButton::clicked, this, [this, stacked]() {
        stacked->setCurrentIndex(1);
        qApp->processEvents();
        this->parent->close();
    });
    layout1->addWidget(closeBtn);

    cancelBtn = new MyPushButton(tr("Return to Edah"), this);
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setIcon(QIcon(":/img/arrow-back.svg"));
    connect(cancelBtn, &MyPushButton::clicked, this, &ClosingPopup::close);
    layout1->addWidget(cancelBtn);

    stacked->addWidget(first);

    QWidget *second = new QWidget(this);
    QHBoxLayout *layout2 = new QHBoxLayout(second);

    QLabel *text = new QLabel(tr("Closing Edah..."), this);
    text->setObjectName("text");
    text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    text->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    layout2->addWidget(text);

    stacked->addWidget(second);
}

void ClosingPopup::resizeEvent(QResizeEvent *event)
{
    this->recalcSizes(event->size());
}

void ClosingPopup::recalcSizes(const QSize &size)
{
    Q_UNUSED(size)

    this->setStyleSheet(QString("#text {"
                                "   font-size: %1px;"
                                "   color: white;"
                                "}"
                                "#shutdownBtn, #closeBtn_, #cancelBtn {"
                                "   font-size: %1px;"
                                "   padding-top: %2px;"
                                "   padding-bottom: %2px;"
                                "}")
                        .arg(qMax(1, parent->height()/24))
                        .arg(qMax(1, parent->height()/96)));

    QSize iconSize(parent->height()/24, parent->height()/24);
    shutdownBtn->setIconSize(iconSize);
    closeBtn->setIconSize(iconSize);
    cancelBtn->setIconSize(iconSize);
}

void ClosingPopup::shutdown()
{
    qApp->quit();

    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        logger->write("OpenProcessToken() failed");
        return;
    }

    // Get the LUID for the shutdown privilege.

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
                         &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process.

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
                          (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS)
    {
        logger->write("AdjustTokenPrivileges() failed");
        return;
    }

    // Shut down the system and force all applications to close.

    if (!ExitWindowsEx(EWX_SHUTDOWN /*| EWX_HYBRID_SHUTDOWN TODO */,
                       SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
    {
        logger->write("ExitWindowsEx() failed");
        return;
    }
}
