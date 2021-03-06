/*
 * Copyright (C) 2010,2012,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGGENERATEREVOKE_H
#define KGPGGENERATEREVOKE_H

#include "kgpgtransaction.h"

#include "core/kgpgkey.h"

#include <QUrl>
#include <QObject>

class QString;

/**
 * @brief generate a revokation certificate
 */
class KGpgGenerateRevoke: public KGpgTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgGenerateRevoke)
	KGpgGenerateRevoke() Q_DECL_EQ_DELETE;
public:
	/**
	 * @brief KGpgGenerateRevoke's constructor
	 * @param parent parent object
	 * @param keyID key fingerprint to generate revokation certificate for
	 * @param revokeUrl place to store the certificate (may be empty)
	 * @param reason revokation reason
	 * @param description text description for revokation
	 */
	KGpgGenerateRevoke(QObject *parent, const QString &keyID, const QUrl &revokeUrl, const int reason, const QString &description);
	virtual ~KGpgGenerateRevoke();

	/**
	 * @brief returns the revokation certificate
	 */
	const QString &getOutput() const;

signals:
	void revokeCertificate(const QString &cert);

protected:
	bool preStart() Q_DECL_OVERRIDE;
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;
	ts_boolanswer boolQuestion(const QString &line) Q_DECL_OVERRIDE;
	void finish() Q_DECL_OVERRIDE;
	bool passphraseReceived() Q_DECL_OVERRIDE;
	ts_boolanswer confirmOverwrite (QUrl &currentFile) Q_DECL_OVERRIDE;

private:
	QString m_keyid;
	QUrl m_revUrl;
	int m_reason;
	QString m_description;
	QString m_output;
};

#endif // KGPGGENERATEREVOKE_H
