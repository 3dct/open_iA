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

#include "open_iA_Core_export.h"

#include <QAtomicInteger>
#include <QMap>
#include <QWidget>

class iAAbortListener;
class iADurationEstimator;
class iAProgress;

//! A simple widget showing a list of currently running jobs and their progress.
class open_iA_Core_API iAJobListView : public QWidget
{
	Q_OBJECT
public:
	iAJobListView();
	template <typename TaskT>
	void addJob(QString name, iAProgress* p, TaskT* t, iAAbortListener* abortListener = nullptr,
		QSharedPointer<iADurationEstimator> estimator =	QSharedPointer<iADurationEstimator>());
signals:
	void allJobsDone();
private:
	QWidget* addJobWidget(QString name, iAProgress* p, iAAbortListener* abortListener,
		QSharedPointer<iADurationEstimator> estimator);
	
	QAtomicInteger<int> m_runningJobs;
	QWidget* m_insideWidget;
	QMap<QWidget*, QSharedPointer<iADurationEstimator>> m_estimators;
};

template <typename TaskT>
void iAJobListView::addJob(QString name, iAProgress* p, TaskT* t, iAAbortListener* abortListener,
	QSharedPointer<iADurationEstimator> estimator)
{
	m_runningJobs.fetchAndAddOrdered(1);
	auto jobWidget = addJobWidget(name, p, abortListener, estimator);
	connect(t, &TaskT::finished, [this, jobWidget]()
		{
			int oldJobCount = m_runningJobs.fetchAndAddOrdered(-1);
			if (oldJobCount == 1)
			{
				emit allJobsDone();
			}
			m_estimators.remove(jobWidget);
			jobWidget->deleteLater();
		});
	show();
}
