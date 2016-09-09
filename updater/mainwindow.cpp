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

#include "mainwindow.h"

#include <QCoreApplication>
#include <QCloseEvent>
#include <QMessageBox>

#include <libedah/logger.h>
#include <libedah/utils.h>

#include <Windows.h>

const int totalSteps = 3;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), canClose(true), lastProgress(0)
{
    logger = new Logger;
    utils = new Utils;

    settings = new QSettings("Lukasz Matczak", "Edah", this);

    QString localeStr = settings->value("lang", "").toString();
    if(localeStr.isEmpty())
    {
        localeStr = QLocale::system().name().left(2);
    }
    if(localeStr != "en")
    {
        translator.load(QLocale(localeStr), "lang", ".", ":/lang");
    }

    qApp->installTranslator(&translator);

    this->setWindowTitle(tr("Updating"));
    this->resize(339, 77);
    this->setWindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    central = new QWidget(this);
    this->setCentralWidget(central);

    layout = new QVBoxLayout(central);

    stepLbl = new QLabel(tr("Step 1/3: Downloading"), this);
    layout->addWidget(stepLbl);

    progressLbl = new QLabel(tr("Downloaded %1 KB of %2 KB (%3 KB/s)").arg(0).arg(0).arg(0), this);
    layout->addWidget(progressLbl);

    progressBar = new QProgressBar(this);
    layout->addWidget(progressBar);

    updater = new Updater;
    updater->setInstallDir(QCoreApplication::arguments().at(1));
    connect(this, &MainWindow::doUpdate, updater, &Updater::doUpdate);
    connect(updater, &Updater::progress, this, &MainWindow::progress);
    connect(updater, &Updater::verFailed, this, [this](){ QMessageBox::critical(this, tr("Error!"), tr("An error occured during downloading updates!")); done(); });
    connect(updater, &Updater::updateFinished, this, [this](){ done(); });
    updater->moveToThread(&updaterThread);
    updaterThread.start();

    emit doUpdate();

    lastUpdateTime.start();
}

MainWindow::~MainWindow()
{

}

void MainWindow::progress(int step, int curr, int max)
{
    switch (step)
    {
    case 0:
    {
        stepLbl->setText(tr("Step 1/3: Downloading"));

        int transfer = ((curr-lastProgress)/1024.0)/((lastUpdateTime.elapsed())/1000.0);
        lastUpdateTime.restart();
        lastProgress = curr;
        progressLbl->setText(tr("Downloaded %1 KB of %2 KB (%3 KB/s)").arg(curr/1024).arg(max/1024).arg(transfer));

        progressBar->setMaximum(max);
        progressBar->setValue(curr);
        break;
    }

    case 1:
    {
        stepLbl->setText(tr("Step 2/3: Checking"));
        progressLbl->setText(tr("Verifying file %1 of %2").arg(curr).arg(max));

        progressBar->setMaximum(max);
        progressBar->setValue(curr);
        break;
    }

    case 2:
    {
        this->canClose = false;
        this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        this->show();
        stepLbl->setText(tr("Step 3/3: Installing"));
        progressLbl->setText(tr("Copying file %1 of %2").arg(curr).arg(max));

        progressBar->setMaximum(max);
        progressBar->setValue(curr);
        break;
    }
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if(!canClose)
        e->ignore();
}

// based on: https://blogs.msdn.microsoft.com/aaron_margosis/2009/06/06/faq-how-do-i-start-a-program-as-the-desktop-user-from-an-elevated-app/
// Definition of the function this sample is all about.
// The szApp, szCmdLine, szCurrDir, si, and pi parameters are passed directly to CreateProcessWithTokenW.
// sErrorInfo returns text describing any error that occurs.
// Returns "true" on success, "false" on any error.
// It is up to the caller to close the HANDLEs returned in the PROCESS_INFORMATION structure.
bool RunAsDesktopUser(
        __in    const wchar_t *       szApp,
        __in    wchar_t *             szCmdLine,
        __in    const wchar_t *       szCurrDir,
        __in    STARTUPINFOW &        si,
        __inout PROCESS_INFORMATION & pi)
{
    HANDLE hShellProcess = NULL, hShellProcessToken = NULL, hPrimaryToken = NULL;
    HWND hwnd = NULL;
    DWORD dwPID = 0;
    BOOL ret;
    DWORD dwLastErr;

    // Enable SeIncreaseQuotaPrivilege in this process.  (This won't work if current process is not elevated.)
    HANDLE hProcessToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hProcessToken))
    {
        return false;
    }
    else
    {
        TOKEN_PRIVILEGES tkp;
        tkp.PrivilegeCount = 1;
        LookupPrivilegeValueW(NULL, SE_INCREASE_QUOTA_NAME, &tkp.Privileges[0].Luid);
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hProcessToken, FALSE, &tkp, 0, NULL, NULL);
        dwLastErr = GetLastError();
        CloseHandle(hProcessToken);
        if (ERROR_SUCCESS != dwLastErr)
        {
            return false;
        }
    }

    // Get an HWND representing the desktop shell.
    // CAVEATS:  This will fail if the shell is not running (crashed or terminated), or the default shell has been
    // replaced with a custom shell.  This also won't return what you probably want if Explorer has been terminated and
    // restarted elevated.
    hwnd = GetShellWindow();
    if (NULL == hwnd)
    {
        return false;
    }

    // Get the PID of the desktop shell process.
    GetWindowThreadProcessId(hwnd, &dwPID);
    if (0 == dwPID)
    {
        return false;
    }

    // Open the desktop shell process in order to query it (get the token)
    hShellProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
    if (!hShellProcess)
    {
        return false;
    }

    // From this point down, we have handles to close, so make sure to clean up.

    bool retval = false;
    // Get the process token of the desktop shell.
    ret = OpenProcessToken(hShellProcess, TOKEN_DUPLICATE, &hShellProcessToken);
    if (!ret)
    {
        goto cleanup;
    }

    // Duplicate the shell's process token to get a primary token.
    // Based on experimentation, this is the minimal set of rights required for CreateProcessWithTokenW (contrary to current documentation).
    const DWORD dwTokenRights = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID;
    ret = DuplicateTokenEx(hShellProcessToken, dwTokenRights, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken);
    if (!ret)
    {
        goto cleanup;
    }

    // Start the target process with the new token.
    ret = CreateProcessWithTokenW(
                hPrimaryToken,
                0,
                szApp,
                szCmdLine,
                0,
                NULL,
                szCurrDir,
                &si,
                &pi);
    if (!ret)
    {
        goto cleanup;
    }

    retval = true;

cleanup:
    // Clean up resources
    CloseHandle(hShellProcessToken);
    CloseHandle(hPrimaryToken);
    CloseHandle(hShellProcess);
    return retval;
}

void MainWindow::done()
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    SecureZeroMemory(&si, sizeof(si));
    SecureZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (RunAsDesktopUser(
                (PCWSTR)(QCoreApplication::arguments().at(1)+"\\Edah.exe").utf16(),
                nullptr,
                (PCWSTR)QCoreApplication::arguments().at(1).utf16(),
                si,
                pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    this->canClose = true;
    qApp->quit();
}
