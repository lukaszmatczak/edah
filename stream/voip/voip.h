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

#ifndef VOIP_H
#define VOIP_H

//#include <ptlib.h>
#include <opal.h>
#include <opal/manager.h>
#include <ep/pcss.h>

class SIPEndPoint;

class MyManager : public OpalManager
{
    PCLASSINFO(MyManager, OpalManager)

public:
    ~MyManager();
    PBoolean Initialise(PArgList & args);
    void Main(PArgList & args);
    void OnEstablishedCall(OpalCall & call);

private:
    SIPEndPoint *sipEP;
    PString callToken;

    HANDLE hMapFile;
    OpalPCSSConnection *PCSSptr;
    bool PCSSvalid;

    PString id;
    PString code;
    PString port;

    PString numberOfProcess;
    PString join;

    volatile struct
    {
        bool end;
        int recLevel;
        int playLevel;
    } *shared;
};

class VoipProcess : public PProcess
{
  PCLASSINFO(VoipProcess, PProcess)

  public:
    VoipProcess();

    void Main();

  protected:
    MyManager * opal;
};

#endif // VOIP_H
