/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2008,2009,2010,2011,2012,2013,2014
 *               Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2016 Andrius Štikonas <andrius@stikonas.eu>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgpg.h"

#include "gpgproc.h"
#include "kgpgsettings.h"
#include "keysmanager.h"
#include "kgpg_interface.h"
#include "kgpgexternalactions.h"
#include "kgpginterface.h"
#include "core/images.h"
#include "editor/kgpgeditor.h"
#include "transactions/kgpgimport.h"

#include <QCommandLineParser>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTextStream>

#include <KMessageBox>
#include <KWindowSystem>

using namespace KgpgCore;

KGpgApp::KGpgApp(int &argc, char **argv)
             : QApplication(argc, argv),
	     running(false),
	     w(Q_NULLPTR),
	     s_keyManager(0)
{
}

KGpgApp::~KGpgApp()
{
	delete s_keyManager;
}

void KGpgApp::slotHandleQuit()
{
	s_keyManager->saveToggleOpts();
	quit();
}

void KGpgApp::assistantOver(const QString &defaultKeyId)
{
	if (!defaultKeyId.isEmpty())
		s_keyManager->slotSetDefaultKey(defaultKeyId);

	s_keyManager->show();
	s_keyManager->raise();
}

int KGpgApp::newInstance(QCommandLineParser& parser)
{
	if (!running) {
		running = true;

		const QString gpgError = GPGProc::getGpgStartupError(KGpgSettings::gpgBinaryPath());
		if (!gpgError.isEmpty()) {
			running = false;
			KMessageBox::detailedError(0, i18n("GnuPG failed to start.<br />You must fix the GnuPG error first before running KGpg."), gpgError, i18n("GnuPG error"));
			QApplication::quit();
		}

		s_keyManager = new KeysManager();

		w = new KGpgExternalActions(s_keyManager, s_keyManager->getModel());

		connect(s_keyManager, &KeysManager::readAgainOptions, w, &KGpgExternalActions::readOptions);
		connect(w, &KGpgExternalActions::updateDefault, this, &KGpgApp::assistantOver);
		connect(w, &KGpgExternalActions::createNewKey, s_keyManager, &KeysManager::slotGenerateKey);

		const QString gpgPath = KGpgSettings::gpgConfigPath();

		if (!gpgPath.isEmpty()) {
			const int gpgver = GPGProc::gpgVersion(GPGProc::gpgVersionString(KGpgSettings::gpgBinaryPath()));

			// Warn if sign of a properly running gpg-agent cannot be determined
			// The environment variable has been removed in GnuPG 2.1, the agent is started internally by
			// any program part of GnuPG that needs it, so simply assume everything is fine.
			if ((gpgver < 0x20100) && KgpgInterface::getGpgBoolSetting(QLatin1String("use-agent"), gpgPath) &&
					qgetenv("GPG_AGENT_INFO").isEmpty())
				KMessageBox::sorry(0, i18n("<qt>The use of <b>GnuPG Agent</b> is enabled in GnuPG's configuration file (%1).<br />"
					"However, the agent does not seem to be running. This could result in problems with signing/decryption.<br />"
					"Please disable GnuPG Agent from KGpg settings, or fix the agent.</qt>", gpgPath));
		}
	}

	// parsing of command line args
	if (parser.isSet("k") || (!KGpgSettings::showSystray() && parser.positionalArguments().isEmpty() && !parser.isSet("d"))) {
		s_keyManager->show();
		KWindowSystem::setOnDesktop(s_keyManager->winId(), KWindowSystem::currentDesktop());  //set on the current desktop
		KWindowSystem::unminimizeWindow(s_keyManager->winId());  //de-iconify window
		s_keyManager->raise();  // set on top
	} else if (parser.isSet("d")) {
		s_keyManager->slotOpenEditor();
		s_keyManager->hide();
	} else {
		QList<QUrl> urlList;

		const QStringList positionalArguments = parser.positionalArguments();
		for (const QString &arg : positionalArguments)
			urlList.append(QUrl::fromLocalFile(arg));

		bool directoryInside = false;
		foreach (const QUrl &url, urlList) {
			QMimeDatabase db;
			if (db.mimeTypeForUrl(url).name() == QLatin1String( "inode/directory" )) {
				directoryInside = true;
				break;
			}
		}

		if (parser.isSet("e")) {
			if (urlList.isEmpty())
				KMessageBox::sorry(0, i18n("No files given."));
			else if (!directoryInside)
				KGpgExternalActions::encryptFiles(s_keyManager, urlList);
			else
				KGpgExternalActions::encryptFolders(s_keyManager, urlList);
		} else if (parser.isSet("s")) {
			if (urlList.isEmpty())
				KMessageBox::sorry(0, i18n("No files given."));
			else if (!directoryInside)
				w->showDroppedFile(urlList.first());
			else
				KMessageBox::sorry(0, i18n("Cannot decrypt and show folder."));
		} else if (parser.isSet("S")) {
			if (urlList.isEmpty())
				KMessageBox::sorry(0, i18n("No files given."));
			else if (!directoryInside)
				KGpgExternalActions::signFiles(s_keyManager, urlList);
			else
				KMessageBox::sorry(0, i18n("Cannot sign folder."));
		} else if (parser.isSet("V") != 0) {
			if (urlList.isEmpty())
				KMessageBox::sorry(0, i18n("No files given."));
			else if (!directoryInside)
				w->verifyFile(urlList.first());
			else
				KMessageBox::sorry(0, i18n("Cannot verify folder."));
		} else {
			if (directoryInside && (urlList.count() > 1)) {
				KMessageBox::sorry(0, i18n("Unable to perform requested operation.\nPlease select only one folder, or several files, but do not mix files and folders."));
				return 0;
			}

			if (urlList.isEmpty()) {
				/* do nothing */
			} else if (urlList.first().fileName().endsWith(QLatin1String(".sig"))) {
				w->verifyFile(urlList.first());
			} else {
				bool haskeys = false;
				bool hastext = false;
				foreach (const QUrl &url, urlList) {
					QFile qfile(url.path());
					if (qfile.open(QIODevice::ReadOnly)) {
						const int probelen = 4096;
						QTextStream t(&qfile);
						QString probetext(t.read(probelen));
						qfile.close();

						if (KGpgImport::isKey(probetext, probetext.length() == probelen))
							haskeys = true;
						else
							hastext = true;
					}
				}

				if (hastext) {
					KGpgExternalActions::decryptFiles(s_keyManager, urlList);
				} else if (haskeys) {
					s_keyManager->slotImport(urlList);
				}
			}
		}
	}

	return 0;
}
