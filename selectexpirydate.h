/**
 * Copyright (C) 2002 Jean-Baptiste Mardelle <bj@altern.org>
 * Copyright (C) 2007 Jimmy Gilles <jimmygilles@gmail.com>
 * Copyright (C) 2008 Rolf Eike Beer <kde@opensource.sf-tec.de>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SELECTEXPIRYDATA_H
#define SELECTEXPIRYDATA_H

#include <QDate>

#include <KDialog>

class KDatePicker;
class QCheckBox;

/**
 * @brief shows a dialog to choose expiry date
 *
 * This dialog allows the user to choose a expiry date or set a
 * checkbox to select that the given item will never expire.
 */
class SelectExpiryDate : public KDialog
{
	Q_OBJECT

public:
	explicit SelectExpiryDate(QWidget *parent = 0, QDate date = QDate());

	QDate date() const;

private slots:
	void slotCheckDate(const QDate &date);
	void slotEnableDate(const bool ison);

private:
	QCheckBox *m_unlimited;
	KDatePicker *m_datepicker;
};

#endif /* SELECTEXPIRYDATA_H */
