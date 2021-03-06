/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2008,2009,2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KGPGEXTERNALACTIONS_H
#define _KGPGEXTERNALACTIONS_H

#include <QObject>
#include <QPointer>
#include <QStringList>

#include <QUrl>

class KeysManager;
class KGpgFirstAssistant;
class KGpgItemModel;
class KJob;
class QKeySequence;
class QTemporaryFile;
class QString;

/**
 * @brief handle actions called from outside the application
 *
 * This class takes care about everything sent to us from outside the
 * application, e.g. command line arguments given on startup.
 */
class KGpgExternalActions : public QObject
{
	Q_OBJECT

public:
	KGpgExternalActions(KeysManager *parent, KGpgItemModel *model);
	~KGpgExternalActions();

	void showDroppedFile(const QUrl &file);
	void verifyFile(QUrl url);

	/**
	 * @brief create a detached signature for the given files
	 */
	static void signFiles(KeysManager* parent, const QList<QUrl> &urls);

	static void decryptFiles(KeysManager* parent, const QList<QUrl>& urls);
	static void encryptFolders(KeysManager* parent, const QList<QUrl> &urls);

	/**
	 * @brief create a new object, encrypt the given files, and destroy the object
	 */
	static void encryptFiles(KeysManager* parent, const QList<QUrl>& urls);

	void readOptions();
signals:
	void createNewKey();
	void updateDefault(QString);

private:
	QStringList customDecrypt;
	QPointer<KGpgFirstAssistant> m_assistant;
	int compressionScheme;
	KGpgItemModel *m_model;
	QTemporaryFile *m_kgpgfoldertmp;

	void startAssistant();
	void firstRun();

	QList<QUrl> m_decryptionFailed;
	KeysManager *m_keysmanager;
	QList<QUrl> droppedUrls;

	QKeySequence goDefaultKey() const;
	void decryptFile(QList<QUrl> urls);

private slots:
	void startFolderEncode();
	void slotSaveOptionsPath();
	void slotVerificationDone(int result);
	void help();
	void slotSetCompression(int cp);
	void slotDecryptionDone(int status);
	void slotFolderFinished(KJob *job);
	void slotSignFiles();
	void slotEncryptionKeySelected();
};

#endif /* _KGPGEXTERNALACTIONS_H */
