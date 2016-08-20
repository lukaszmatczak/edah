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

#include "multilangstring.h"

QString MultilangString::lang = "en";

MultilangString::MultilangString()
{

}

MultilangString::MultilangString(const MultilangString &other)
{
    data = other.data;
}

MultilangString::MultilangString(const MultilangString &&other)
{
    data = other.data;
}

QString MultilangString::toString() const
{
    return data[lang];
}

MultilangString &MultilangString::operator =(const MultilangString &other)
{
    data = other.data;

    return *this;
}

MultilangString::operator QString() const
{
    return data[lang];
}

QString &MultilangString::operator [](const QString &l)
{
    return data[l];
}

void MultilangString::setLang(const QString &l)
{
    lang = l;
}

MultilangString MultilangString::fromJson(const QJsonObject &json)
{
    MultilangString string;

    for(int i=0; i<json.size(); i++)
    {
        QString lang = json.keys()[i];
        string[lang] = json.value(lang).toString();
    }

    return string;
}
