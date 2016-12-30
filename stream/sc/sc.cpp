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

#include "sc.h"

#include <iostream>

std::string getopt(int argc, char **argv, const std::string &arg)
{
    for(int i=1; i<argc; i++)
    {
        std::string argvn = std::string(argv[i]);
        if(argvn.find(arg) == 0)
        {
            return argvn.substr(arg.length());
        }
    }

    return "";
}

BOOL CALLBACK RecordProc(HRECORD handle, const void *buffer, DWORD length, void *user)
{
    return TRUE;
}

int main(int argc, char **argv)
{
    std::cerr << "Start\n";

    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "sc_shared");

    if(hMapFile == NULL)
    {
        return -10;
    }

    Shared *shared;

    shared = (Shared*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*shared));

    if(shared == NULL)
    {
        return -11;
    }

    std::string recDev = getopt(argc, argv, "--recDev=");
    //int sc_version = std::stoi(getopt(argc, argv, "--version="));

    BASS_SetConfig(BASS_CONFIG_UNICODE, true);

    int recDevNo = -1;

    BASS_DEVICEINFO info;

    for(int i=0; BASS_RecordGetDeviceInfo(i, &info); i++)
    {
        if(recDev == info.name)
        {
            recDevNo = i;
            break;
        }
    }

    if(!BASS_RecordInit(recDevNo))
    {
        int code = BASS_ErrorGetCode();
        std::string text = "BASS error: " + std::to_string(code);

        if(code == 23)
            text += "\nProblem with device: \"" + recDev + "\"!";

        std::cerr << text << "\n";

        return -1;
    }

    int sampleRate = std::stoi(getopt(argc, argv, "--samplerate="));
    int channels = std::stoi(getopt(argc, argv, "--channels="));

    recStream = BASS_RecordStart(sampleRate, channels, 0, RecordProc, nullptr);

#ifdef _WIN32
    std::string lameBin = "plugins/stream/lame.exe";
#else
    std::string lameBin = "lame";
#endif

    HENCODE encoder = BASS_Encode_Start(recStream,
                                        (lameBin + " -b " + getopt(argc, argv, "--bitrate=") + " -").c_str(),
                                        0,
                                        NULL,
                                        0);

    bool ok = BASS_Encode_CastInit(encoder,
                         getopt(argc, argv, "--url=").c_str(),
                         getopt(argc, argv, "--password=").c_str(),
                         BASS_ENCODE_TYPE_MP3,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         std::stoi(getopt(argc, argv, "--bitrate=")),
                         FALSE);

    if(!ok)
    {
        int code = BASS_ErrorGetCode();
        std::string text = "BASS error: " + std::to_string(code);

        std::cerr << text << "\n";

        return -2;
    }

    std::cerr << "OK\n";

    while(!shared->end)
    {
        BASS_CHANNELINFO info;

        if(BASS_ChannelGetInfo(recStream, &info))
        {
            DWORD flags = info.chans == 1 ? BASS_LEVEL_MONO : BASS_LEVEL_STEREO;
            BASS_ChannelGetLevelEx(recStream, shared->levels, 0.035f, flags);
        }

        Sleep(35);
    }

    std::cerr << "End\n";

    UnmapViewOfFile((LPCVOID)shared);
    CloseHandle(hMapFile);

    return 0;
}
