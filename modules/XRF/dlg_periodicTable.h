/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAPeriodicTableWidget.h"

#include <QDockWidget>
#include "ui_PeriodicTable.h"
#include "iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDockWidget, Ui_PeriodicTable> dlg_periodicTableContainer;

class iAElementSelectionListener;

class dlg_periodicTable : public dlg_periodicTableContainer
{
	Q_OBJECT
public:
	dlg_periodicTable(QWidget *parent): dlg_periodicTableContainer(parent)
	{
		m_periodicTableWidget = new iAPeriodicTableWidget(parent);
		m_periodicTableWidget->setObjectName(QString::fromUtf8("PeriodicTable"));
		setWidget(m_periodicTableWidget);
	}
	void setConcentration(QString const & elementName, double percentage, QColor const & color)
	{
		m_periodicTableWidget->setConcentration(elementName, percentage, color);
	}
	void setListener(QSharedPointer<iAElementSelectionListener> listener)
	{
		m_periodicTableWidget->setListener(listener);
	}
	int GetCurrentElement() const
	{
		return m_periodicTableWidget->GetCurrentElement();
	}
private:
	iAPeriodicTableWidget* m_periodicTableWidget;
};
