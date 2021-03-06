/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "ui_samplings.h"

#include <qthelper/iAQTtoUIConnector.h>

class iASamplingResults;

class QStandardItemModel;

typedef iAQTtoUIConnector<QDockWidget, Ui_samplings> dlgSamplingsUI;

class dlg_samplings : public dlgSamplingsUI
{
	Q_OBJECT
public:
	typedef QSharedPointer<iASamplingResults> SamplingResultPtr;
	dlg_samplings();
	void Add(SamplingResultPtr samplingResults);
	SamplingResultPtr GetSampling(int idx);
	int SamplingCount() const;
	QSharedPointer<QVector<SamplingResultPtr> > GetSamplings();
public slots:
	void Remove();
signals:
	void AddSampling();
private:
	QStandardItemModel* m_itemModel;
	QSharedPointer<QVector<SamplingResultPtr> > m_samplings;
};