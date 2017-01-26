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

#include "voip.h"

#include <iostream>
#include <sip/sip.h>
//#include <ep/pcss.h>

#include <Iphlpapi.h>

#define APP_MANUF "Lukasz Matczak"
#define APP_NAME "Edah_VoIP"

int exitCode = 0;

PCREATE_PROCESS(VoipProcess)

VoipProcess::VoipProcess() : PProcess(APP_MANUF, APP_NAME)
{
}

MyManager::~MyManager()
{
    ClearCall(callToken);

    UnmapViewOfFile((LPCVOID)shared);
    CloseHandle(hMapFile);
}

PBoolean MyManager::Initialise(PArgList & args)
{
    //PTrace::Initialise(6, "C:\\Users\\lukasz\\Desktop\\log.txt", PTrace::Timestamp|PTrace::FileAndLine|PTrace::Blocks);

    srand(time(NULL));

    PString user = args.GetOptionString("u");
    PString password = args.GetOptionString("p");

    id = args.GetOptionString("i");
    code = args.GetOptionString("c");
    port = args.GetOptionString("s");

    PString playDev = args.GetOptionString("d");
    PString recDev = args.GetOptionString("r");

    cerr << "Logging in";

    sipEP = new SIPEndPoint(*this);
    sipEP->SetProxy("sip.ipfon.pl:"+port, user, password);
    sipEP->SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);

    SIPRegister::Params params;
    params.m_addressOfRecord =  user + "@sip.ipfon.pl:" + port;
    params.m_password = password;

    //SIPURL::DefaultPort = SIPURL::DefaultPort-(numberOfProcess.AsInteger()-1);

    PString aor;

    sipEP->StartListeners(PStringArray());

    sipEP->Register(params, aor, true);

    int i=0;
    while(!sipEP->IsRegistered(aor))
    {
        SIPEndPoint::RegistrationStatus status;
        if(!sipEP->GetRegistrationStatus(aor, status) || (i>100))
        {
            cerr << "Unable to log in";
            exitCode = -2;
            return false;
        }
        i++;
        PThread::Sleep(100);
    }

    PStringList prefixes = GetPrefixNames(sipEP);

    for (PINDEX i = 0; i < prefixes.GetSize(); ++i)
    {
        AddRouteEntry(prefixes[i] + ":.* = " + "pc:");
    }

    OpalPCSSEndPoint * pcss = new OpalPCSSEndPoint(*this);

    pcss->SetSoundChannelPlayDevice(playDev);
    pcss->SetSoundChannelRecordDevice(recDev);

    OpalSilenceDetector::Params param = OpalSilenceDetector::NoSilenceDetection;
    SetSilenceDetectParams(param);

    return true;
}

void MyManager::Main(PArgList & args)
{
    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "voip_shared");

    if(hMapFile == NULL)
    {
        exitCode = -4;
        return;
    }

    shared = (decltype(shared))MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*shared));

    if(shared == NULL)
    {
        exitCode = -5;
        return;
    }

    PCSSvalid = false;

    cerr << "Connecting";

    SetUpCall("pc:*", "sip:"+id+"@sip.ipfon.pl:"+port, callToken);

    if ((FindCallWithLock(callToken)) != NULL)
    {
        while(HasCall(callToken) && !shared->end)
        {
            for(int i=0; (i<10) && (!shared->end); i++)
            {
                shared->playLevel = PCSSvalid ? PCSSptr->GetAudioSignalLevel(false)*4 : 0;
                shared->recLevel = PCSSvalid ? PCSSptr->GetAudioSignalLevel(true)*4 : 0;

                if(PCSSvalid && (args.GetOptionString("d") == "Mute"))
                {
                    PCSSptr->SetAudioVolume(false, 0);
                }
                PThread::Sleep(50);
            }
        }

        PCSSvalid = false;
        shared->playLevel = shared->recLevel = 0;
    }
}

void MyManager::OnEstablishedCall(OpalCall & call)
{
    cerr << "Joining a conference call";

    PSafePtr<SIPConnection> SIPconn = call.GetConnectionAs<SIPConnection>();
    PSafePtr<OpalPCSSConnection> PCSSconn = call.GetConnectionAs<OpalPCSSConnection>();

    PCSSptr = PCSSconn;
    PCSSvalid = true;

    if(SIPconn != NULL)
    {
        PThread::Sleep(5000);

        if(code.GetLength() != 4)
        {
            cerr << "Incorrect PIN";
            exitCode = -10;
            shared->end = true;
            return;
        }

        for(int i=0; i<4; i++)
        {
            SIPconn->SendUserInputTone(code[i], 100);
            PThread::Sleep(200);
        }

        SIPconn->SendUserInputTone('#', 100);

        //PThread::Sleep(1000);
        int cnt = 0;
        for(int i=0; i<1000; i++)
        {
            int level = PCSSptr->GetAudioSignalLevel(false);

            if((level > 2900) && (level < 3600))
                cnt++;
            else
                cnt = max(cnt-1, 0);

            if(cnt > 20)
            {
                cerr << "OK";
                return;
            }
            PThread::Sleep(10);
        }
    }
    else
    {
        exitCode = -6;
        shared->end = true;
        return;
    }

    cerr << "Unable to join";

    exitCode = -11;
    shared->end = true;
    return;
}

void VoipProcess::Main()
{
    PArgList & args = GetArguments();
    args.Parse("u:p:s:i:c:d:r:");

    if(!(args.HasOption("u") && args.HasOption("p") &&
         args.HasOption("s") && args.HasOption("i") &&
         args.HasOption("c") && args.HasOption("d") &&
         args.HasOption("r")))
    {
        cerr << "Incorrect configuration";
        SetTerminationValue(-3);
        return;
    }

    opal = new MyManager;

    if (opal->Initialise(args))
        opal->Main(args);

    delete opal;

    SetTerminationValue(exitCode);
}
