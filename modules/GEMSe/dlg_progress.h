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

#include "ui_progress.h"

#include "iAAbortListener.h"
#include "iADurationEstimator.h"

#include <qthelper/iAQTtoUIConnector.h>

typedef iAQTtoUIConnector<QDockWidget, Ui_progress> dlg_progressUI;

//! Class to display progress of a task running in the background
//! ToDo: Merge with iAJobListView in FIAKER!
class dlg_progress : public dlg_progressUI
{
	Q_OBJECT
public:
	dlg_progress(QWidget *parentWidget,
		QSharedPointer<iADurationEstimator const> estimator,
		QSharedPointer<iAAbortListener> abort,
		QString const & caption);
public slots:
	void setProgress(int progress);
	void setStatus(QString const & status);
	void abort();
private:
	QSharedPointer<iADurationEstimator const> m_durationEstimator;
	QSharedPointer<iAAbortListener> m_abortListener;
};
