/*
 * Copyright (C) 2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgencrypt.h"

#include <gpgproc.h>
#include <kgpginterface.h>

#include <KLocale>

static QStringList trustOptions(const QString &binary)
{
	const int gpgver = KgpgInterface::gpgVersion(KgpgInterface::gpgVersionString(binary));
	QStringList args;
	if (gpgver >= 0x10302)
		args << QLatin1String("--trust-model")
				<< QLatin1String("always");
	else
		args << QLatin1String("--always-trust");

	return args;
}

KGpgEncrypt::KGpgEncrypt(QObject *parent, const QStringList &userIds, const QString &text, const EncryptOptions &options, const QStringList &extraOptions)
	: KGpgTextOrFileTransaction(parent, text),
	m_fileIndex(-1),
	m_options(options),
	m_userIds(userIds),
	m_extraOptions(extraOptions)
{
	if ((m_options & AllowUntrustedEncryption) && !m_userIds.isEmpty())
		m_extraOptions << trustOptions(getProcess()->program().at(0));
}

KGpgEncrypt::KGpgEncrypt(QObject *parent, const QStringList &userIds, const KUrl::List &files, const EncryptOptions &options, const QStringList &extraOptions)
	: KGpgTextOrFileTransaction(parent, files),
	m_fileIndex(0),
	m_options(options),
	m_userIds(userIds),
	m_extraOptions(extraOptions)
{
	if ((m_options & AllowUntrustedEncryption) && !m_userIds.isEmpty())
		m_extraOptions << trustOptions(getProcess()->program().at(0));
}

KGpgEncrypt::~KGpgEncrypt()
{
}

QStringList
KGpgEncrypt::command() const
{
	QStringList ret = m_extraOptions;

	if (m_options.testFlag(AsciiArmored))
		ret << QLatin1String("--armor");

	if (m_userIds.isEmpty()) {
		ret << QLatin1String( "--symmetric" );
	} else {
		if (m_options.testFlag(HideKeyId))
			ret << QLatin1String("--throw-keyid");

		foreach (const QString &uid, m_userIds)
			ret << QLatin1String( "--recipient" ) << uid;
		ret << QLatin1String( "--encrypt" );
	}

	return ret;
}

QStringList
KGpgEncrypt::encryptedText() const
{
	QStringList result;
	int txtlength = 0;

	foreach (const QString &line, getMessages())
		if (!line.startsWith(QLatin1String("[GNUPG:] "))) {
			result.append(line);
			txtlength += line.length() + 1;
		}

	return result;
}

bool
KGpgEncrypt::nextLine(const QString &line)
{
	const KUrl::List &inputFiles = getInputFiles();

	if (line.startsWith(QLatin1String("[GNUPG:] MISSING_PASSPHRASE"))) {
		setSuccess(KGpgTransaction::TS_BAD_PASSPHRASE);
		return true;
	}

	if (!inputFiles.isEmpty()) {
		static const QString encStart = QLatin1String("[GNUPG:] FILE_START 2 ");
		static const QString encDone = QLatin1String("[GNUPG:] FILE_DONE");

		if (line.startsWith(encStart)) {
			m_currentFile = line.mid(encStart.length());
			emit statusMessage(i18nc("Status message 'Encrypting <filename>' (operation starts)", "Encrypting %1", m_currentFile));
			emit infoProgress(2 * m_fileIndex + 1, inputFiles.count() * 2);
		} else if (line == encDone) {
			emit statusMessage(i18nc("Status message 'Encrypted <filename>' (operation was completed)", "Encrypted %1", m_currentFile));
			m_fileIndex++;
			emit infoProgress(2 * m_fileIndex, inputFiles.count() * 2);
		}
	}

	return KGpgTextOrFileTransaction::nextLine(line);
}

#include "kgpgencrypt.moc"