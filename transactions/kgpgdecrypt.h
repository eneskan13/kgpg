/*
 * Copyright (C) 2010,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGDECRYPT_H
#define KGPGDECRYPT_H

#include <QObject>

#include <QUrl>

#include "kgpgtextorfiletransaction.h"

class QStringList;

/**
 * @brief decrypt the given text or files
 */
class KGpgDecrypt: public KGpgTextOrFileTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgDecrypt)
	KGpgDecrypt() Q_DECL_EQ_DELETE;
public:
	/**
	 * @brief decrypt given text
	 * @param parent parent object
	 * @param text text to decrypt
	 */
	explicit KGpgDecrypt(QObject *parent, const QString &text = QString());

	/**
	 * @brief decrypt file(s)
	 * @param parent parent object
	 * @param files list of file locations to decrypt
	 */
	KGpgDecrypt(QObject *parent, const QList<QUrl> &files);

	/**
	 * @brief decrypt file to given output filename
	 * @param parent parent object
	 * @param infile name of file to decrypt
	 * @param outfile name of file to write output to (will be overwritten)
	 */
	KGpgDecrypt(QObject *parent, const QUrl &infile, const QUrl &outfile);

	/**
	 * @brief destructor
	 */
	virtual ~KGpgDecrypt();

	/**
	 * @brief get decryption result
	 * @return decrypted text
	 */
	QStringList decryptedText() const;

	/**
	 * @brief check if the given text contains an encoded message
	 * @param text text to check
	 * @param startPos if not Q_NULLPTR start offset of encoded text will be returned here
	 * @param endPos if not Q_NULLPTR end offset of encoded text will be returned here
	 */
	static bool isEncryptedText(const QString &text, int *startPos = Q_NULLPTR, int *endPos = Q_NULLPTR);

protected:
	QStringList command() const Q_DECL_OVERRIDE;
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;

private:
	int m_fileIndex;
	int m_plainLength;	///< length of decrypted plain text if given by GnuPG
	const QString m_outFilename;	///< name of file to write output to
};

#endif // KGPGDECRYPT_H
