/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAguibase_export.h"

#include <QStack>
#include <QWidget>

class iAAbortListener;
class iADurationEstimator;
class iAJob;
class iAProgress;

//! A list of currently running jobs and their progress.
//! Implemented as singleton, since there is only one such list per program instance.
class iAguibase_API iAJobListView : public QWidget
{
	Q_OBJECT
public:
	//! Access to the single instance of this class (singleton).
	static iAJobListView* get();
	//! Typical way of adding a job. Specify its name, the progress observer,
	//! the task (to whose finished signal the removal of the entry is connected),
	//! and optional abort listener and progress estimator
	//! @anchor addJobParams
	//! @param name the name of the job/operation that is run;
	//!        it is always shown at the top of the list entry, in bold
	//! @param p the progress observer that reports on the job's progress;
	//!        its progress signal connects to the job entries' progress bar,
	//!        and the setStatus signal connects to a secondary text information
	//!        shown below the name of the task (in non-bold font)
	//! @param t a "handle" of the job - templated so that any type which has a
	//!        "finished" signal can be passed in; this could be a QThread,
	//!        or a QFuture as returned by runAsync (see iARunAsync.h)
	//! @param abortListener optional listener to presses of the abort button;
	//!        the abort button will be disabled if the default nullptr value is passed here
	//! @param estimator an estimator for the remaining duration of the job, in case
	//!        an unusual way of computing the elapsed and/or estimated remaining time
	//!        is required. By default, a simple estimation based on the current
	//!        finished percentage and the time elapsed so far will be used.
	void addJob(QString name, iAProgress* p, QObject* t, iAAbortListener* abortListener = nullptr,
		QSharedPointer<iADurationEstimator> estimator = QSharedPointer<iADurationEstimator>());
	//! Add a job bound to the life time of the returned object.
	//! useful for situations where no finished signal can be connected
	//! (e.g. if a synchronous operation is run)
	//! the job will stay in the list as long as somebody holds a reference
	//! to the returned QObject; in a typical use case, this method will be
	//! called at the start of the method performing the operation, and when
	//! the variable holding the returned pointer goes out of scope at the
	//! end of the method, the job will be removed automatically from the list.
	//! Example:
	//!
	//! 
	//!     #include "iAJobListView.h"
	//!     #include "iAProgress.h"
	//!     ...
	//!     void myComputeMethod()
	//!     {
	//!         iAProgress p;
	//!         auto jobListHandle = iAJobListView::get()->addJob("Compute Job", &p);
	//!         while (computing)
	//!         {
	//!             // ... do computation
	//!             p.emitProgress(percent);
	//!         }
	//!     }  // here, jobListHandle goes out of scope, and the job will be automatically removed from the list!
	//!
	//! For comments on the parameters, see @ref addJobParams "the other addJob variant".
#if __cplusplus >= 201703L
	[[nodiscard]]
#endif
	QSharedPointer<QObject>	addJob(QString name, iAProgress* p, iAAbortListener* abortListener = nullptr,
		QSharedPointer<iADurationEstimator> estimator = QSharedPointer<iADurationEstimator>());
	//! Destructor, automatically cancels any still running jobs.
	//! Even for jobs it's not able to cancel, removes their finish action.
	~iAJobListView();
signals:
	//! Emitted when all jobs are done.
	//! Is used in main window to hide widget as it means that no more jobs are currently running.
	void allJobsDone();
	//! Emitted when a job is added.
	//! Used in main window to show widget, as it indicates that there is at least on currently running job.
	void jobAdded(QObject* o);
	//! Used to link the GUI part of adding a job to the backend part.
	//! Required for decoupling these two to allow adding jobs from a backend thread
	void newJobSignal();
private slots:
	//! Linked to newJobSignal, does the GUI part of adding a new job.
	void newJobSlot();
private:
	//! Prevent creation - singleton pattern
	iAJobListView();
	QWidget* addJobWidget(QSharedPointer<iAJob> j);
	//! The container widget for all job entries
	QWidget* m_insideWidget;
	//! List of jobs pending to be added (needed to be able to add jobs also from non-GUI-threads
	QStack<QSharedPointer<iAJob>> m_pendingJobs;
	//! Currently running jobs
	QVector<QSharedPointer<iAJob>> m_jobs;
};
