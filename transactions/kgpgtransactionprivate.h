/*
 * Copyright (C) 2008,2009,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGTRANSACTIONPRIVATE_H
#define KGPGTRANSACTIONPRIVATE_H

#include "gpgproc.h"

#include <QUrl>

#include <KPasswordDialog>
#include <KNewPasswordDialog>

class KGpgTransaction;
class KGpgSignTransactionHelper;

class KGpgTransactionPrivate : public QObject {
public:
	KGpgTransactionPrivate(KGpgTransaction *parent, bool allowChaining);
	~KGpgTransactionPrivate();

	KGpgTransaction *m_parent;
	GPGProc *m_process;
	KGpgTransaction *m_inputTransaction;
	KNewPasswordDialog *m_newPasswordDialog;
	KPasswordDialog *m_passwordDialog;
	int m_success;
	int m_tries;
	QString m_description;
	bool m_chainingAllowed;

	QStringList m_idhints;

	QUrl m_overwriteUrl;	///< the file to overwrite or it's new name

	void slotReadReady();
	void slotProcessExited();
	void slotProcessStarted();
	void slotInputTransactionDone(int result);
	void slotPassphraseEntered(const QString &passphrase);

	/**
	 * @brief a slot to handle the case that the passphrase entry was aborted by the user
	 *
	 * This will delete the sender as well as do the internal passphrase aborted handling.
	 */
	void slotPassphraseAborted();
	/**
	 * @brief do the internal passphrase aborted handling
	 */
	void handlePassphraseAborted();

	QList<int *> m_argRefs;
	bool m_inputProcessDone;
	int m_inputProcessResult;
	bool m_ownProcessFinished;

	/**
	 * terminate GnuPG session
	 */
	void sendQuit(void);

	void write(const QByteArray &a);

	static const QStringList &hintNames(void);

private:
	void processDone();

	unsigned int m_quitTries;	///< how many times we tried to quit
	QStringList m_quitLines;	///< what we received after we tried to quit
};

#endif // KGPGTRANSACTIONPRIVATE_H
