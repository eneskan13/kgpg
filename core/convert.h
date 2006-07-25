/***************************************************************************
 *   Copyright (C) 2006 by Jimmy Gilles                                    *
 *   jimmygilles@gmail.com                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef CONVERT_H
#define CONVERT_H

#include <QString>
#include <QColor>

#include "kgpgkey.h"

namespace KgpgCore
{

class Convert
{
public:
    static QString toString(const KeyAlgo &algorithme);
    static QString toString(const KeyOwnerTrust &ownertrust);
    static QString toString(const KeyTrust &trust);
    static QString toString(const QDate &date);
    static QColor toColor(const KeyTrust &trust);
};

} // namespace KgpgCore

#endif // CONVERT_H