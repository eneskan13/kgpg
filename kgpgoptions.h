/***************************************************************************
                          kgpgoptions.h  -  description
                             -------------------
    begin                : Mon Jul 8 2002
    copyright            : (C) 2002 by y0k0
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
#ifndef KGPGOPTIONS_H
#define KGPGOPTIONS_H

#include <qlayout.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qpushbutton.h>
#include <qhbuttongroup.h>
#include <qvbuttongroup.h>

#include <kdialogbase.h>
#include <kcombobox.h>
#include <kurlrequester.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <ktoolbar.h>
//#include <kstatusbar.h>

#include <kconfig.h>
#include <klocale.h>
#include "kgpgoption.h"



class kgpgOptions : public KgpgOptionDialog
{
        Q_OBJECT
public:
        kgpgOptions(QWidget *parent=0, const char *name=0);
        ~kgpgOptions();
        QStringList names,ids;

private:
        KConfig *config;
        QString confPath,defaultKeyID;

private slots:
        void slotSaveServer();
        void listkey();
        QString namecode(QString kid);
        QString idcode(QString kname);
        void slotOk();
        void slotInstallDecrypt(QString mimetype);
        void slotInstallSign(QString mimetype);
        void slotRemoveMenu(QString menu);
        void slotEditServer();
        void slotAddServer();
        void slotRemoveServer();
        void reloadServer();
        void slotDefaultServer();
public slots:
        void checkMimes();
signals:

};

#endif
