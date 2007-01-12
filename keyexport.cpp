/***************************************************************************
                          keyexport.cpp  -  description
                             -------------------
    begin                : Thu Jul 4 2002
    copyright          : (C) 2002 by Jean-Baptiste Mardelle
    email                : bj@altern.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "keyexport.h"

KeyExport::KeyExport( QWidget* parent )
    : QWidget( parent ), Ui_KeyExport()
{
    setupUi( this );
    connect(checkFile, SIGNAL(toggled(bool)), this,SLOT(checkFile_toggled(bool)));
}

void KeyExport::checkFile_toggled( bool isOn)
{
    newFilename->setEnabled(isOn);
}
#include "keyexport.moc"