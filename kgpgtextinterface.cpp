/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpgtextinterface.h"

#include <QTextCodec>
#include <QFile>

#include <KLocale>

#include "kgpginterface.h"
#include "gpgproc.h"
#include "kgpgsettings.h"

class KGpgTextInterfacePrivate
{
public:
	KGpgTextInterfacePrivate(QObject *parent);

	GPGProc * const m_process;
	bool m_badpassword;
	bool m_anonymous;
	int m_step;
	QString m_message;
	QString m_signID;
	QStringList m_userIDs;
	QStringList m_gpgopts;
	KUrl m_file;
	KUrl::List m_files;
	KUrl::List m_errfiles;

	void updateIDs(QByteArray txt);
	bool symPassphrase();
	bool gpgPassphrase();
	void signFile(const KUrl &);

	void appendLog(const QByteArray &a);
	QString log() const;

private:
	bool m_consoleUtf8;
	QString m_log;
};

//krazy:cond=strings
static bool isUtf8Lang(const QByteArray &lc)
{
	return lc.endsWith("UTF-8");
}
//krazy:endcond=strings

KGpgTextInterfacePrivate::KGpgTextInterfacePrivate(QObject *parent)
	: m_process(new GPGProc(parent)),
	m_badpassword(false),
	m_anonymous(false),
	m_step(3),
	m_consoleUtf8(true)
{
	const QByteArray lcMess = qgetenv("LC_MESSAGES");

	if (!lcMess.isEmpty()) {
		m_consoleUtf8 = isUtf8Lang(lcMess);
	} else {
		const QByteArray lcAll = qgetenv("LC_ALL");
		if (!lcAll.isEmpty()) {
			m_consoleUtf8 = isUtf8Lang(lcAll);
		} else {
			const QByteArray lang = qgetenv("LANG");
			if (!lang.isEmpty())
				m_consoleUtf8 = isUtf8Lang(lang);
		}
	}

	*m_process <<
			QLatin1String("--status-fd=1") <<
			QLatin1String("--command-fd=0");
}

void
KGpgTextInterfacePrivate::updateIDs(QByteArray txt)
{
	int cut = txt.indexOf(' ', 22);
	txt.remove(0, cut);

	int pos = txt.indexOf('(');
	if (pos >= 0)
		txt.remove(pos, txt.indexOf(')', pos));

	txt.replace('<', "&lt;");
	QString s = GPGProc::recode(txt);

	if (!m_userIDs.contains(s))
		m_userIDs << s;
}

bool
KGpgTextInterfacePrivate::symPassphrase()
{
	if (KgpgInterface::sendPassphrase(i18n("Enter passphrase (symmetrical encryption)"), m_process)) {
		m_process->kill();
		return true;
	} else {
		return false;
	}
}

bool
KGpgTextInterfacePrivate::gpgPassphrase()
{
	QString s;

	if (m_userIDs.isEmpty())
		s = i18n("[No user id found]");
	else
		s = m_userIDs.join( i18n(" or " ));

	QString passdlgmessage;
	if (m_anonymous)
		passdlgmessage = i18n("<p><b>No user id found</b>. Trying all secret keys.</p>");
	if ((m_step < 3) && !m_anonymous)
		passdlgmessage = i18np("<p><b>Bad passphrase</b>. You have 1 try left.</p>",
		                       "<p><b>Bad passphrase</b>. You have %1 tries left.</p>", m_step);
	if (m_userIDs.isEmpty())
		passdlgmessage += i18n("Enter passphrase");
	else
		passdlgmessage += i18n("Enter passphrase for <b>%1</b>", s);

	if (KgpgInterface::sendPassphrase(passdlgmessage, m_process, false)) {
		m_process->kill();
		return true;
	}

	m_step--;

	return false;
}

void
KGpgTextInterfacePrivate::signFile(const KUrl &file)
{
	*m_process << QLatin1String( "--command-fd=0" ) << QLatin1String( "-u" ) << m_signID;

	*m_process << m_gpgopts;

	if (m_gpgopts.contains(QLatin1String( "--detach-sign" )) && !m_gpgopts.contains( QLatin1String( "--output" )))
		*m_process << QLatin1String( "--output" ) << file.path() + QLatin1String( ".sig" );
	*m_process << file.path();

	m_process->start();
}

void
KGpgTextInterfacePrivate::appendLog(const QByteArray &a)
{
	if (m_consoleUtf8)
		m_log += QString::fromUtf8(a);
	else
		m_log += QString::fromLocal8Bit(a);
}

QString
KGpgTextInterfacePrivate::log() const
{
	return m_log;
}

KGpgTextInterface::KGpgTextInterface(QObject *parent)
	: QObject(parent),
	d(new KGpgTextInterfacePrivate(parent))
{
}

KGpgTextInterface::~KGpgTextInterface()
{
	delete d->m_process;
	delete d;
}

void
KGpgTextInterface::verifyText(const QString &text)
{
	QTextCodec *codec = QTextCodec::codecForLocale();
	if (codec->canEncode(text))
		d->m_message = text;
	else
		d->m_message = QLatin1String( text.toUtf8() );

	*d->m_process << QLatin1String( "--verify" );

	connect(d->m_process, SIGNAL(readReady()), this, SLOT(readVerify()));
	connect(d->m_process, SIGNAL(processExited()), this, SLOT(verifyTextFin()));
	d->m_process->start();

	d->m_process->write(d->m_message.toAscii());
	d->m_message.clear();
	d->m_process->closeWriteChannel();
}

void
KGpgTextInterface::verifyTextFin()
{
	if (!d->m_userIDs.isEmpty()) {
		emit txtVerifyMissingSignature(d->m_userIDs.first());
	} else {
		if (d->m_signID.isEmpty())
			d->m_signID = i18n("No signature found.");

		emit txtVerifyFinished(d->m_signID, d->m_message);
	}
}

void
KGpgTextInterface::KgpgVerifyFile(const KUrl &sigUrl, const KUrl &srcUrl)
{
	d->m_file = sigUrl;

	*d->m_process << QLatin1String( "--verify" );
	if (!srcUrl.isEmpty())
		*d->m_process << srcUrl.path();
	*d->m_process << sigUrl.path();

	connect(d->m_process, SIGNAL(lineReadyStandardOutput()), this, SLOT(readVerify()));
	connect(d->m_process, SIGNAL(processExited()), this, SLOT(verifyfin()));
	d->m_process->start();
}

//krazy:cond=strings
void
KGpgTextInterface::readVerify()
{
	QByteArray line;

	while (d->m_process->readLineStandardOutput(&line)) {
		d->m_message += GPGProc::recode(line) + QLatin1Char( '\n' );
		if (line.contains( "GET_" ))
			d->m_process->write("quit\n");

		if (!line.startsWith("[GNUPG:] "))
			continue;
		line.remove(0, 9);
		if (line.startsWith("UNEXPECTED") || line.startsWith("NODATA")) {
			d->m_signID = i18n("No signature found.");
		} else if (line.startsWith("GOODSIG")) {
			int sigpos = line.indexOf( ' ' , 8);
			d->m_signID = i18n("<qt>Good signature from:<br /><b>%1</b><br />Key ID: %2<br /></qt>",
					GPGProc::recode(line.mid(sigpos + 1).replace('<', "&lt;")),
					QString::fromAscii(line.mid(8, sigpos - 8)));
		} else if (line.startsWith("BADSIG")) {
			int sigpos = line.indexOf( ' ', 7);
			d->m_signID = i18n("<qt><b>BAD signature</b> from:<br /> %1<br />Key id: %2<br /><br /><b>The file is corrupted</b><br /></qt>",
					GPGProc::recode(line.mid(sigpos + 1).replace('<', "&lt;")),
					QString::fromAscii(line.mid(7, sigpos - 7)));
		} else if (line.startsWith("NO_PUBKEY")) {
			d->m_userIDs << QLatin1String(line.remove(0, line.indexOf(' ')));
		} else  if (line.startsWith("TRUST_UNDEFINED")) {
			d->m_signID += i18n("<qt>The signature is valid, but the key is untrusted<br /></qt>");
		} else if (line.startsWith("TRUST_ULTIMATE")) {
			d->m_signID += i18n("<qt>The signature is valid, and the key is ultimately trusted<br /></qt>");
		}
	}
}
//krazy:endcond=strings

void
KGpgTextInterface::verifyfin()
{
	if (d->m_userIDs.isEmpty()) {
		emit verifyfinished(d->m_signID, d->m_message);
	} else {
		emit verifyquerykey(d->m_userIDs.first(), d->m_file.fileName());
	}
}

// signatures
void
KGpgTextInterface::signFiles(const QString &keyID, const KUrl::List &srcUrls, const QStringList &options)
{
	d->m_files = srcUrls;
	d->m_step = 0;
	d->m_signID = keyID;
	d->m_gpgopts = options;

	slotSignFile(0);
}

void
KGpgTextInterface::slotSignFile(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step);

	d->m_process->resetProcess();
	d->signFile(d->m_files.at(d->m_step));

	if (++d->m_step == d->m_files.count()) {
		connect(d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotSignFinished(int)));
	} else {
		connect(d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotSignFile(int)));
	}
}

void
KGpgTextInterface::slotSignFinished(int err)
{
	if (err != 0)
		d->m_errfiles << d->m_files.at(d->m_step - 1);

	emit fileSignFinished(d->m_errfiles);
}

#include "kgpgtextinterface.moc"
