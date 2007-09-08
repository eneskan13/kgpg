/***************************************************************************
                          keyservers.h  -  description
                             -------------------
    begin                : Tue Nov 26 2002
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

#ifndef KEYSERVERS_H
#define KEYSERVERS_H

#include <KDialog>

#include "kgpginterface.h"
#include "ui_searchres.h"
#include "ui_keyserver.h"

class Q3ListViewItem;

class KConfig;
class K3ProcIO;
class K3Process;

class keyServerWidget : public QWidget, public Ui::keyServerWidget
{
public:
    keyServerWidget(QWidget *parent = 0) : QWidget(parent)
    {
        setupUi(this);
    }
};

class searchRes : public QWidget, public Ui::searchRes
{
public:
    searchRes(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
    }
};

class ConnectionDialog : public KDialog
{
    Q_OBJECT

public:
    ConnectionDialog(QWidget *parent = 0);
};

class KeyServer : public KDialog
{
    Q_OBJECT

public:
    explicit KeyServer(QWidget *parent = 0, const bool &modal = false, const bool &autoclose = false);

    void refreshKeys(QStringList *keys);

signals:
    void importFinished(QStringList);

public slots:
    void slotImport();
    void slotAbortDownload(KgpgInterface *interface);

    void slotExport(const QString &keyId);
    void slotAbortExport();
    void slotSearch();
    void slotAbortSearch();
    void slotSetText(const QString &text);
    void slotTextChanged(const QString &text);
    void slotSetExportAttribute(const QString *attr);
    void slotEnableProxyI(const bool &on);
    void slotEnableProxyE(const bool &on);
    void slotSetKeyserver(const QString &server);

    void transferKeyID();
    void slotPreImport();
    void slotPreExport();

    void slotOk();
    void handleQuit();

private slots:
    void slotReadKeys(KgpgCore::KgpgKeyList list, KgpgInterface *interface);
    void slotDownloadKeysFinished(QList<int> results, QStringList keys, bool imported, QString log, KgpgInterface *interface);

    void slotImportRead(K3ProcIO *p);
    void slotExportResult(K3Process *p);
    void slotSearchRead(K3ProcIO *p);
    void slotSearchResult(K3Process *p);

private:
    /**
     * Returns the server list.
     * The first item is the server configured in gpg.
     */
    static QStringList getServerList();

private:
    KDialog *m_importpop;
    QString m_readmessage;
    Q3ListViewItem *m_kitem;

    KDialog *m_dialogserver;
    K3ProcIO *m_importproc;
    K3ProcIO *m_exportproc;
    K3ProcIO *m_searchproc;

    keyServerWidget *page;
    searchRes *m_listpop;

    int m_count;
    uint m_keynumbers;
    bool m_autoclose;
    QString m_keyid;
    QString expattr;

    void CreateUidEntry(void);
    K3ProcIO *createGPGProc(QStringList *keys);
};

#endif // KEYSERVERS_H
