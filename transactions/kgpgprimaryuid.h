/*
 * Copyright (C) 2009,2013 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGPGPRIMARYUID_H
#define KGPGPRIMARYUID_H

#include <QObject>

#include "kgpguidtransaction.h"

class KGpgUidNode;

/**
 * @brief transaction class to change the primary user id of a key
 */
class KGpgPrimaryUid: public KGpgUidTransaction {
	Q_OBJECT

	Q_DISABLE_COPY(KGpgPrimaryUid)
	KGpgPrimaryUid() Q_DECL_EQ_DELETE;

public:
	/**
	 * @brief constructor
	 * @param parent parent object
	 * @param uid user id to become new primary one
	 */
	KGpgPrimaryUid(QObject *parent, KGpgUidNode *uid);
	/**
	 * @brief destructor
	 */
	virtual ~KGpgPrimaryUid();

protected:
	bool nextLine(const QString &line) Q_DECL_OVERRIDE;
	bool passphraseReceived() Q_DECL_OVERRIDE;

private:
	int m_fixargs;
};

#endif // KGPGPRIMARYUID_H
