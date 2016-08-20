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


#ifndef MULTILANGSTRING_H
#define MULTILANGSTRING_H

#include <QMap>
#include <QJsonObject>

class MultilangString
{
public:
    explicit MultilangString();
    MultilangString(const MultilangString &other);
    MultilangString(const MultilangString &&other);

    QString toString() const;

    operator QString() const;
    MultilangString &operator =(const MultilangString &other);
    QString &operator [](const QString &l);

    static MultilangString fromJson(const QJsonObject &json);

private:
    static void setLang(const QString &l);

    static QString lang;
    QMap<QString, QString> data;

    friend class MainWindow;
};

#endif // MULTILANGSTRING_H
