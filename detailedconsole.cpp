/***************************************************************************
 *   Copyright (C) 2003 by bjUTHOR$                                        *
 *   bj@altern.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

 
 #include "detailedconsole.h"
 
 
 KDetailedConsole::KDetailedConsole(QWidget *parent, const char *name,QString boxLabel,QString errormessage): KDialogBase(parent,name,true,i18n("Sorry"),KDialogBase::Details|KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok)
  {
   QWidget *page = new QWidget( this );
   setMainWidget(page);
QVBoxLayout *vbox=new QVBoxLayout(page,3);
  
 QLabel *lab1=new QLabel(page);
 lab1->setText(boxLabel);
  
QVGroupBox *detailsGroup = new QVGroupBox( i18n("Details:"), page);
QLabel *labdetails = new QLabel(errormessage,detailsGroup);
labdetails->setMinimumSize(labdetails->sizeHint());
setDetailsWidget(detailsGroup);
vbox->addWidget(lab1);

}

KDetailedConsole::~KDetailedConsole()
{}

#include "detailedconsole.moc"
