/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <QtConcurrent>

//! Runs a function asynchronously and triggers another function when finished.
//! Note: See notes on context parameter about taking care of aborting the function
//!         (or at least its finished handling) when the program is terminated!
//! @param runner the function to run asynchronously, can be a lambda or a member function
//! @param finish the function to run after the main task (runner) is done; it's
//!         linked via signal-slot mechanism, so this runs in whatever thread
//!         it "belongs" to
//! @param context the object in whose context the finish function should run.
//!         (should? serve two purposes: context provides thread that finish function runs in; and when context deleted, so are connections)
//!         TODO: STOP THREAD - through future watcher?
//!         second purpose behind it is that once the object context links to is destroyed,
//!         so is the connection; used to handle for example the case of terminating
//!         the application; if you pass a context object here that automatically gets
//!         destroyed with the application (e.g. some GUI element), then your finished
//!         handler also gets disconnected; if it's a long-running operation, you
//!         might need to take other measures that the operation itself is aborted too
//!         (see e.g. iAJobListView who calls abort on any jobs added to it which have
//!         an abort listener registered)
template <typename RunnerT, typename FinishT>
QFutureWatcher<void>* runAsync(RunnerT runner, FinishT finish, QObject* context)
{
	auto futureWatcher = new QFutureWatcher<void>(context); // shouldn' I use context as parent here?
	QObject::connect(futureWatcher, &QFutureWatcher<void>::finished, context, finish);
	QObject::connect(futureWatcher, &QFutureWatcher<void>::finished, futureWatcher, &QFutureWatcher<void>::deleteLater);
	auto future = QtConcurrent::run(runner);
	futureWatcher->setFuture(future);
	return futureWatcher;
}
