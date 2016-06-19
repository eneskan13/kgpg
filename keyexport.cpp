/*
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007,2011 Rolf Eike Beer <kde@opensource.sf-tec.de>
 * Copyright (C) 2011 Luis Ángel Fernández Fernández <laffdez@gmail.com>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keyexport.h"
#include <KConfigGroup>
#include <KMessageBox>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

KeyExport::KeyExport(QWidget *parent, const QStringList &keyservers)
	: QDialog(parent),
	Ui_KeyExport()
{
	setupUi(this);

	buttonBox->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	newFilename->setWindowTitle(i18n("Save File"));
	newFilename->setMode(KFile::File);

	if (!keyservers.isEmpty()) {
		checkServer->setEnabled(true);
		checkServer->setToolTip(QString());
		destServer->addItems(keyservers);
	}
}

void KeyExport::accept()
{
	if (checkFile->isChecked()) {
		if (QFile::exists(newFilename->url().path().simplified())) {
			const QString message = i18n("Overwrite existing file %1?", newFilename->url().fileName());
			int result = KMessageBox::warningContinueCancel(this, message, QString(), KStandardGuiItem::overwrite());
			if (KMessageBox::Cancel == result)
				return;
		}
	}

	QDialog::accept();
}
