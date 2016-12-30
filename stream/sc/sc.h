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

#ifndef SC_H
#define SC_H

#include <string>

#include <bass.h>
#include <bassenc.h>

struct Shared
{
    bool end;
    float levels[2];
};

std::string getopt(int argc, char **argv, const std::string &arg);
BOOL CALLBACK RecordProc(HRECORD handle, const void *buffer, DWORD length, void *user);
int main(int argc, char **argv);

HRECORD recStream;

#endif // SC_H
