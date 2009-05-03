/***************************************************************************
                          keyservers.cpp  -  description
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

#include "keyservers.h"

#include <QCursor>
#include <QLabel>
#include <QTextCodec>

#include <KConfig>
#include <KMessageBox>
#include <KLocale>
#include <KDebug>
#include <KDateTime>

#include "keylistproxymodel.h"
#include "kgpginterface.h"
#include "kgpgitemmodel.h"
#include "kgpgsearchresultmodel.h"
#include "kgpgsettings.h"
#include "detailedconsole.h"
#include "convert.h"
#include "gpgproc.h"
#include "kgpgkeyservergettransaction.h"

using namespace KgpgCore;

ConnectionDialog::ConnectionDialog(QWidget *parent)
                : KProgressDialog(parent, i18n("Keyserver"), i18n("<b>Connecting to the server...</b>"))
{
	setModal(true);

	progressBar()->setRange(0, 0);
	show();
}

KeyServer::KeyServer(QWidget *parent, KGpgItemModel *model, const bool autoclose)
	: KDialog(parent),
	m_searchproc(NULL),
	page(new keyServerWidget()),
	m_resultmodel(NULL),
	m_itemmodel(new KeyListProxyModel(this, KeyListProxyModel::SingleColumnIdFirst))
{
	setCaption(i18n("Key Server"));
	setButtons(Close);
	setModal(false);

	m_autoclose = autoclose;
	m_filtermodel.setSortCaseSensitivity(Qt::CaseInsensitive);
	m_filtermodel.setDynamicSortFilter(true);
	m_filtermodel.setFilterKeyColumn(0);

	setMainWidget(page);

	const QStringList serverlist(getServerList());
	page->kCBexportks->addItems(serverlist);
	page->kCBimportks->addItems(serverlist);
	page->kLEimportid->setFocus();

	connect(page->Buttonimport, SIGNAL(clicked()), SLOT(slotImport()));
	connect(page->Buttonsearch, SIGNAL(clicked()), SLOT(slotSearch()));
	connect(page->Buttonexport, SIGNAL(clicked()), SLOT(slotPreExport()));
	connect(page->kLEimportid,  SIGNAL(returnPressed()), SLOT(slotSearch()));
	connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
	connect(page->cBproxyI, SIGNAL(toggled(bool)), SLOT(slotEnableProxyI(bool)));
	connect(page->cBproxyE, SIGNAL(toggled(bool)), SLOT(slotEnableProxyE(bool)));
	connect(page->kLEimportid, SIGNAL(textChanged(const QString &)), SLOT(slotTextChanged(const QString &)));

	page->cBproxyI->setChecked(KGpgSettings::useProxy());
	page->cBproxyE->setChecked(KGpgSettings::useProxy());

	const QString httpproxy(qgetenv("http_proxy"));
	if (!httpproxy.isEmpty()) {
		page->cBproxyI->setEnabled(true);
		page->cBproxyE->setEnabled(true);
		page->kLEproxyI->setText(httpproxy);
		page->kLEproxyE->setText(httpproxy);
	}

	page->Buttonimport->setEnabled(!page->kLEimportid->text().isEmpty());
	page->Buttonsearch->setEnabled(!page->kLEimportid->text().isEmpty());
	setMinimumSize(sizeHint());

	m_itemmodel->setKeyModel(model);
	m_itemmodel->setTrustFilter(KgpgCore::TRUST_UNDEFINED);
	page->kCBexportkey->setModel(m_itemmodel);
}

KeyServer::~KeyServer()
{
	delete page;
}

void KeyServer::refreshKeys(const QStringList &keys)
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	const QString keyserv(getServerList().first());

	QString proxy;
	if (KGpgSettings::useProxy())
		proxy = qgetenv("http_proxy");

	KGpgRefreshKeys *proc = new KGpgRefreshKeys(this, keyserv, keys, proxy);
	connect(proc, SIGNAL(done(int)), SLOT(slotDownloadKeysFinished(int)));

	m_importpop = new ConnectionDialog(this);
	connect(m_importpop, SIGNAL(cancelClicked()), proc, SLOT(slotAbort()));

	proc->start();
}

void KeyServer::slotImport()
{
	if (page->kCBimportks->currentText().isEmpty())
		return;

	if (page->kLEimportid->text().isEmpty()) {
		KMessageBox::sorry(this, i18n("You must enter a search string."));
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	KGpgReceiveKeys *proc = new KGpgReceiveKeys(this, page->kCBimportks->currentText(), page->kLEimportid->text().simplified().split(' '), page->kLEproxyI->text());
	connect(proc, SIGNAL(done(int)), SLOT(slotDownloadKeysFinished(int)));

	m_importpop = new ConnectionDialog(this);
	connect(m_importpop, SIGNAL(cancelClicked()), proc, SLOT(slotAbort()));

	proc->start();
}

void KeyServer::slotDownloadKeysFinished(int resultcode)
{
	delete m_importpop;
	m_importpop = 0;

	QApplication::restoreOverrideCursor();

	KGpgKeyserverGetTransaction *t = qobject_cast<KGpgKeyserverGetTransaction *>(sender());
	const QStringList log(t->getLog());

	t->deleteLater();

	if (resultcode == KGpgTransaction::TS_USER_ABORTED)
		return;

	QStringList keys;
	QList<int> result;

	foreach (const QString msg, log) {
		if (msg.startsWith("[GNUPG:] IMPORT_OK ")) {
			keys.append(msg.section(' ', 3));
		} else if (msg.startsWith("[GNUPG:] IMPORT_RES ")) {
			QStringList importresult(msg.mid(20).split(' '));
			foreach (const QString n, importresult)
				result.append(n.toInt());
		}
	}

	QString resultmessage;
	if ((resultcode == KGpgTransaction::TS_OK) && (result.count() >= 13)) {
		if (result[0]  != 0)
			resultmessage += i18np("<qt>%1 key processed.<br/></qt>", "<qt>%1 keys processed.<br/></qt>", result[0]);
		if (result[4]  != 0)
			resultmessage += i18np("<qt>%1 key unchanged.<br/></qt>", "<qt>%1 keys unchanged.<br/></qt>", result[4]);
		if (result[7]  != 0)
			resultmessage += i18np("<qt>%1 signature imported.<br/></qt>", "<qt>%1 signatures imported.<br/></qt>", result[7]);
		if (result[1]  != 0)
			resultmessage += i18np("<qt>%1 key without ID.<br/></qt>", "<qt>%1 keys without ID.<br/></qt>", result[1]);
		if (result[3]  != 0)
			resultmessage += i18np("<qt>%1 RSA key imported.<br/></qt>", "<qt>%1 RSA keys imported.<br/></qt>", result[3]);
		if (result[5]  != 0)
			resultmessage += i18np("<qt>%1 user ID imported.<br/></qt>", "<qt>%1 user IDs imported.<br/></qt>", result[5]);
		if (result[6]  != 0)
			resultmessage += i18np("<qt>%1 subkey imported.<br/></qt>", "<qt>%1 subkeys imported.<br/></qt>", result[6]);
		if (result[8]  != 0)
			resultmessage += i18np("<qt>%1 revocation certificate imported.<br/></qt>", "<qt>%1 revocation certificates imported.<br/></qt>", result[8]);
		if (result[9]  != 0)
			resultmessage += i18np("<qt>%1 secret key processed.<br/></qt>", "<qt>%1 secret keys processed.<br/></qt>", result[9]);
		if (result[10] != 0)
			resultmessage += i18np("<qt><b>%1 secret key imported.</b><br/></qt>", "<qt><b>%1 secret keys imported.</b><br/></qt>", result[10]);
		if (result[11] != 0)
			resultmessage += i18np("<qt>%1 secret key unchanged.<br/></qt>", "<qt>%1 secret keys unchanged.<br/></qt>", result[11]);
		if (result[12] != 0)
			resultmessage += i18np("<qt>%1 secret key not imported.<br/></qt>", "<qt>%1 secret keys not imported.<br/></qt>", result[12]);
		if (result[2]  != 0)
			resultmessage += i18np("<qt><b>%1 key imported:</b><br/></qt>", "<qt><b>%1 keys imported:</b><br/></qt>", result[2]);
	} else {
		resultmessage = i18n("No key imported... \nCheck detailed log for more infos");
	}

	if (!keys.empty())
		emit importFinished(keys);

	(void) new KgpgDetailedInfo(0, resultmessage, log.join(QString('\n')), keys);
}

void KeyServer::slotAbort(KgpgInterface *interface)
{
	interface->deleteLater();
	QApplication::restoreOverrideCursor();
	if (m_autoclose)
		close();
}

void KeyServer::slotExport(const QString &keyId)
{
	if (page->kCBexportks->currentText().isEmpty())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	KgpgInterface *interface = new KgpgInterface();
	connect(interface, SIGNAL(uploadKeysFinished(QString, KgpgInterface*)), this, SLOT(slotUploadKeysFinished(QString, KgpgInterface*)));
	connect(interface, SIGNAL(uploadKeysAborted(KgpgInterface*)), this, SLOT(slotAbort(KgpgInterface*)));

	m_importpop = new ConnectionDialog(this);
	connect(m_importpop, SIGNAL(cancelClicked()), interface, SLOT(uploadKeysAbort()));

	interface->uploadKeys(keyId.simplified().split(' '), page->kCBimportks->currentText(), expattr, page->kLEproxyI->text());
}

void KeyServer::slotUploadKeysFinished(QString message, KgpgInterface *interface)
{
	delete m_importpop;
	m_importpop = 0;
	interface->deleteLater();

	QApplication::restoreOverrideCursor();

	if (message.isEmpty())
		message = i18n("Upload to keyserver finished without errors");
	KMessageBox::information(this, message);
}

void KeyServer::slotSearch()
{
	if (page->kCBimportks->currentText().isEmpty())
		return;

	if (page->kLEimportid->text().isEmpty()) {
		KMessageBox::sorry(this, i18n("You must enter a search string."));
		return;
	}

	page->Buttonsearch->setEnabled(false);
	if (m_searchproc)
		return;

	if (m_resultmodel != NULL)
		m_resultmodel->deleteLater();
	m_resultmodel = new KGpgSearchResultModel(this);
	m_filtermodel.setSourceModel(m_resultmodel);

	m_dialogserver = new KDialog(this );
	m_dialogserver->setCaption( i18n("Import Key From Keyserver") );
	m_dialogserver->setButtons( KDialog::Ok | KDialog::Close );
	m_dialogserver->setDefaultButton( KDialog::Ok);
	m_dialogserver->setModal( true );

	m_dialogserver->setButtonText(KDialog::Ok, i18n("&Import"));
	m_dialogserver->enableButtonOk(false);
	m_listpop = new searchRes(0);
	m_listpop->kLVsearch->setModel(&m_filtermodel);
	m_listpop->statusText->setText(i18n("Connecting to the server..."));

	connect(m_listpop->filterEdit, SIGNAL(textChanged(const QString &)), SLOT(slotSetFilterString(const QString &)));
	connect(m_listpop->kLVsearch->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(transferKeyID()));
	connect(m_dialogserver, SIGNAL(okClicked()), this, SLOT(slotPreImport()));
	connect(m_listpop->kLVsearch, SIGNAL(activated(const QModelIndex &)), m_dialogserver, SIGNAL(okClicked()));
	connect(m_dialogserver, SIGNAL(closeClicked()), this, SLOT(handleQuit()));
	connect(m_listpop, SIGNAL(destroyed()), this, SLOT(slotAbortSearch()));

	m_listpop->kLVsearch->setSelectionMode(QAbstractItemView::ExtendedSelection);

	m_count = 0;
	m_readmessage.clear();

	const QString keyserv(page->kCBimportks->currentText());

	bool useproxy = page->cBproxyI->isChecked();
	const QString proxy(page->kLEproxyI->text());

	m_searchproc = new GPGProc(this);

	if (useproxy) {
		m_searchproc->setEnvironment(QStringList("http_proxy=" + proxy));
		*m_searchproc << "--keyserver-options" << "honor-http-proxy";
	} else {
		*m_searchproc << "--keyserver-options" << "no-honor-http-proxy";
	}
	*m_searchproc << "--keyserver" << keyserv << "--status-fd=1" << "--command-fd=0";
	*m_searchproc << "--with-colons"<< "--search-keys"  << page->kLEimportid->text().simplified().toLocal8Bit();

	connect(m_searchproc, SIGNAL(processExited(GPGProc *)), this, SLOT(slotSearchResult(GPGProc *)));
	connect(m_searchproc, SIGNAL(readReady(GPGProc *)), this, SLOT(slotSearchRead(GPGProc *)));
	m_searchproc->start();

	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	m_dialogserver->setMainWidget(m_listpop);
	m_listpop->setMinimumSize(m_listpop->sizeHint());
	m_dialogserver->exec();
}

void KeyServer::slotAbortSearch()
{
	if (m_searchproc != NULL) {
		if (m_searchproc->state() == QProcess::Running) {
			QApplication::restoreOverrideCursor();
			disconnect(m_searchproc, 0, 0, 0);
			m_searchproc->kill();
		}
		delete m_searchproc;
		m_searchproc = NULL;
	}
	page->Buttonsearch->setEnabled(true);

	if (m_dialogserver) {
		delete m_dialogserver;
		m_dialogserver = 0L;
	}
}

void KeyServer::slotSearchRead(GPGProc *p)
{
	QString line;

	while (p->readln(line, true) >= 0) {
		if (line.startsWith("[GNUPG:] GET_LINE keysearch.prompt")) {
			if (m_count < 4)
				p->write("N\n");
			else
				p->write("Q\n");
		} else if (line.startsWith("[GNUPG:] GOT_IT")) {
			m_count++;
			line.clear();
		} else if (!line.isEmpty() && !line.startsWith("[GNUPG:] ")) {
			m_resultmodel->addResultLine(line);
		}
	}
}

void KeyServer::slotSearchResult(GPGProc *)
{
	m_searchproc->deleteLater();
	m_searchproc = NULL;
	page->Buttonsearch->setEnabled(true);

	m_resultmodel->addResultLine(QString());

	m_dialogserver->enableButtonOk(true);
	QApplication::restoreOverrideCursor();

	int keys = m_resultmodel->rowCount(QModelIndex());
	m_listpop->statusText->setText(i18np("Found 1 matching key", "Found %1 matching keys", keys));

	if (keys > 0)
		m_listpop->kLVsearch->selectionModel()->setCurrentIndex(m_resultmodel->index(0,0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void KeyServer::slotSetText(const QString &text)
{
	page->kLEimportid->setText(text);
}

void KeyServer::slotTextChanged(const QString &text)
{
	page->Buttonimport->setEnabled(!text.isEmpty());
	page->Buttonsearch->setEnabled(!text.isEmpty() && (m_searchproc == NULL));
}

void KeyServer::slotSetExportAttribute(const QString &state)
{
	if (state != 0)
		expattr = state;
	else
		expattr.clear();
}

void KeyServer::slotEnableProxyI(const bool &on)
{
	page->kLEproxyI->setEnabled(on);
}

void KeyServer::slotEnableProxyE(const bool &on)
{
	page->kLEproxyE->setEnabled(on);
}

void KeyServer::transferKeyID()
{
	QSet<QString> ids;

	foreach (QModelIndex index, m_listpop->kLVsearch->selectionModel()->selectedIndexes())
		ids << m_resultmodel->idForIndex(m_filtermodel.mapToSource(index));

	const QStringList idlist(ids.toList());
	m_listpop->kLEID->setText(idlist.join(" "));
}

void KeyServer::slotPreImport()
{
	transferKeyID();
	if (m_listpop->kLEID->text().isEmpty()) {
		KMessageBox::sorry(this, i18n("You must choose a key."));
		return;
	}
	page->kLEimportid->setText(m_listpop->kLEID->text());
	m_dialogserver->close();
	slotImport();
}

void KeyServer::slotPreExport()
{
	slotExport(page->kCBexportkey->currentText().section(':', 0, 0));
}

void KeyServer::slotOk()
{
	accept();
}

QStringList KeyServer::getServerList()
{
	QStringList serverList(KGpgSettings::keyServers()); // From kgpg config
	if (!serverList.isEmpty()) {
		serverList.replaceInStrings(QRegExp(" .*"), "");     // Remove kde 3.5 (Default) tag.
		const QString defaultServer(serverList.takeFirst());
		qSort(serverList);
		serverList.prepend(defaultServer);
	}

	return serverList;
}

void KeyServer::handleQuit()
{
	if (m_searchproc != NULL) {
		if (m_searchproc->state() == QProcess::Running) {
			QApplication::restoreOverrideCursor();
			disconnect(m_searchproc, 0, 0, 0);
			m_searchproc->kill();
		}
		delete m_searchproc;
		m_searchproc = NULL;
	}
	m_dialogserver->close();
	page->Buttonsearch->setEnabled(true);
}

void KeyServer::slotSetKeyserver(const QString &server)
{
	page->kCBimportks->setCurrentIndex(page->kCBimportks->findText(server));
}

void KeyServer::slotSetFilterString(const QString &expression)
{
	m_filtermodel.setFilterRegExp(QRegExp(expression, Qt::CaseInsensitive, QRegExp::RegExp2));
}

#include "keyservers.moc"
