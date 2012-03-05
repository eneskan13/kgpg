/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGINTERFACE_H
#define KGPGINTERFACE_H

#include "core/kgpgkey.h"
#include <QStringList>

class KGpgKeyNode;
class KGpgSignableNode;
class KProcess;
class GPGProc;
class QPixmap;
class QString;

/**
 * This class is the interface for gpg.
 */
class KgpgInterface : public QObject
{
    Q_OBJECT

public:
    /**
     * Initialize the class
     */
    KgpgInterface();
    ~KgpgInterface();

    /**
     * @brief get all group names from a GnuPG config file
     * @param configfile names of groups in this file
     * @return list of groups names
     */
    static QStringList getGpgGroupNames(const QString &configfile);
    /**
     * @brief read the key ids for the given group name
     * @param name name of the group
     * @param configfile the GnuPG config file to use
     * @return list of key ids in this group
     */
    static QStringList getGpgGroupSetting(const QString &name, const QString &configfile);
    /**
     * @brief write a group entry with the given keys
     * @param name name of the group
     * @param values key ids to add to group
     * @param configfile the name of the GnuPG config file
     *
     * If a group with the given name already exists it is replaced.
     */
    static void setGpgGroupSetting(const QString &name, const QStringList &values, const QString &configfile);
    /**
     * @brief rename a group entry
     * @param oldName name of the group
     * @param newName new group name to set
     * @param configfile the name of the GnuPG config file
     * @return true if the group was renamed
     */
    static bool renameGroup(const QString &oldName, const QString &newName, const QString &configfile);
    /**
     * @brief remove a group entry
     * @param name name of the group
     * @param configfile GnuPG config file to use
     */
    static void delGpgGroup(const QString &name, const QString &configfile);

    static QString getGpgSetting(const QString &name, const QString &configfile);
    static void setGpgSetting(const QString &name, const QString &value, const QString &url);

    static bool getGpgBoolSetting(const QString &name, const QString &configfile);
    static void setGpgBoolSetting(const QString &name, const bool enable, const QString &url);

    /**
     * @brief ask the user for a passphrase and send it to the given gpg process
     * @param text text is the message that must be displayed in the MessageBox
     * @param process GnuPG process
     * @param isnew if the password is a \e new password that must be confirmed. Default is true
     * @param widget parent widget of this dialog or NULL
     * @return 0 if there is no error
     * @return 1 if there is an error
     */
    static int sendPassphrase(const QString &text, KProcess *process, const bool isnew = true, QWidget *widget = NULL);

    /************** extract public keys **************/
signals:
    void readPublicKeysFinished(KgpgCore::KgpgKeyList);

public:
    KgpgCore::KgpgKeyList readPublicKeys(const bool block = false, const QStringList &ids = QStringList());
    KgpgCore::KgpgKey readSignatures(KGpgKeyNode *node);

private slots:
    void readPublicKeysProcess(GPGProc *p = NULL);
    void readPublicKeysFin(GPGProc *p = NULL, const bool block = false);

private:
	unsigned int m_numberid;
	KgpgCore::KgpgKey m_publickey;
	KgpgCore::KgpgKeyList m_publiclistkeys;

    /*************************************************/


    /************** extract secret keys **************/
public slots:
    KgpgCore::KgpgKeyList readSecretKeys(const QStringList &ids = QStringList());

private slots:
    void readSecretKeysProcess(GPGProc *p);

private:
    bool m_secretactivate;
    KgpgCore::KgpgKey m_secretkey;
    KgpgCore::KgpgKeyList m_secretlistkeys;

    /*************************************************/


    /************** load a photo in a QPixmap **************/
public:
    static QPixmap loadPhoto(const QString &keyid, const QString &uid);

    /*******************************************************/

private:
    // Globals private
    QString log;

    /**
     * @internal structure for communication
     */
    QString output;

    KGpgKeyNode *m_readNode;	///< the node where the signatures are read for
    KGpgSignableNode *m_currentSNode;	///< the current (sub)node signature are read for
};

#endif // KGPGINTERFACE_H
