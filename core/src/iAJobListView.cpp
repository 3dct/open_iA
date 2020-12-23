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
#include "iAJobListView.h"

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iALog.h"
#include "iAPerformanceHelper.h"
#include "iAProgress.h"

#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

#include <chrono>

class iAJobPending
{
public:
	iAJobPending(QString n, iAProgress* p, QObject* o, iAAbortListener* a, QSharedPointer<iADurationEstimator> e):
		name(n), prog(p), obj(o), abrt(a), est(e)
	{
	}
	QString name;
	iAProgress* prog;
	QObject* obj;
	iAAbortListener* abrt;
	QSharedPointer<iADurationEstimator> est;
};

//! Simple estimator: starts the clock as soon as it is created,
//! and estimates remaining time by percentage of completion and elapsed time.
class iAPercentBasedEstimator : public iADurationEstimator
{
public:
	iAPercentBasedEstimator():
		m_start(std::chrono::system_clock::now())
	{
	}
	double elapsed() const override
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now() - m_start).count() / 1e6;
	}
	double estimatedTimeRemaining(double percent) const override
	{
		if (percent == 0 || elapsed() == 0)
		{
			return -1;
		}
		return elapsed() * (100 - percent) / percent;
	}
private:
	std::chrono::system_clock::time_point m_start;
};

iAJobListView* iAJobListView::get()
{
	static iAJobListView* jobList = new iAJobListView();
	return jobList;
}

iAJobListView::iAJobListView():
	m_insideWidget(new QWidget)
{
	m_insideWidget->setProperty("qssClass", "jobList");
	m_insideWidget->setLayout(new QVBoxLayout());
	m_insideWidget->layout()->setContentsMargins(4, 4, 4, 4);
	m_insideWidget->layout()->setSpacing(4);
	m_insideWidget->layout()->setAlignment(Qt::AlignTop);
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(1, 0, 1, 0);
	layout()->setSpacing(0);
	layout()->addWidget(m_insideWidget);
	connect(this, &iAJobListView::newJobSignal, this, &iAJobListView::newJobSlot, Qt::QueuedConnection); // make sure widgets are created in GUI thread
}

QWidget* iAJobListView::addJobWidget(QString name, iAProgress* p, iAAbortListener* abortListener,
	QSharedPointer<iADurationEstimator> estimator)
{
	auto titleLabel = new QLabel(name);
	titleLabel->setProperty("qssClass", "titleLabel");

	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 1000);
	progressBar->setValue(0);

	auto statusLabel = new QLabel("");
	statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	auto elapsedLabel = new QLabel("Elapsed: -");
	elapsedLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	auto remainingLabel = new QLabel("Estimated remaining: unknown");
	remainingLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

	auto statusWidget = new QWidget();
	statusWidget->setLayout(new QVBoxLayout);
	statusWidget->layout()->setContentsMargins(0, 0, 0, 0);
	statusWidget->layout()->setSpacing(2);
	statusWidget->layout()->addWidget(statusLabel);
	statusWidget->layout()->addWidget(elapsedLabel);
	statusWidget->layout()->addWidget(remainingLabel);
	statusWidget->layout()->addWidget(progressBar);

	auto abortButton = new QToolButton();
	abortButton->setIcon(QIcon(":/images/remove.png"));
	abortButton->setEnabled(abortListener);

	auto contentWidget = new QWidget();
	contentWidget->setLayout(new QHBoxLayout);
	contentWidget->layout()->setContentsMargins(0, 0, 0, 0);
	contentWidget->layout()->setSpacing(2);
	contentWidget->layout()->addWidget(statusWidget);
	contentWidget->layout()->addWidget(abortButton);

	auto jobWidget = new QWidget();
	jobWidget->setProperty("qssClass", "jobWidget");
	jobWidget->setLayout(new QVBoxLayout());
	jobWidget->layout()->setContentsMargins(4, 4, 4, 4);
	jobWidget->layout()->setSpacing(4);
	jobWidget->layout()->addWidget(titleLabel);
	jobWidget->layout()->addWidget(contentWidget);

	m_insideWidget->layout()->addWidget(jobWidget);
	
	if (!estimator)
	{
		estimator = QSharedPointer<iADurationEstimator>(new iAPercentBasedEstimator());
	}
	m_estimators.insert(jobWidget, estimator);
	QTimer* timer = new QTimer(jobWidget);
	connect(timer, &QTimer::timeout, jobWidget, [elapsedLabel, estimator] {
		elapsedLabel->setText(QString("Elapsed: %1").arg(formatDuration(estimator->elapsed(), false)));
	});
	timer->start(500);

	if (p)
	{
		connect(p, &iAProgress::progress, jobWidget, [progressBar, estimator, remainingLabel](double value)
			{
				progressBar->setValue(value*10);
				double estRem = estimator->estimatedTimeRemaining(value);
				remainingLabel->setText(
					QString("Estimated remaining: %1").arg((estRem == -1) ? "unknown" : formatDuration(estRem, false)));
			});
		connect(p, &iAProgress::statusChanged, [statusLabel, name](QString const & msg)
			{
				LOG(lvlDebug, QString("Job '%1': %2").arg(name).arg(msg));
				statusLabel->setText(msg);
			});
	}
	if (abortListener)
	{
		connect(abortButton, &QToolButton::clicked, [=]()
			{
				LOG(lvlDebug, QString("Job '%1': Aborted.").arg(name));
				abortButton->setEnabled(false);
				statusLabel->setText("Aborting...");
				if (p)
				{
					QObject::disconnect(p, &iAProgress::statusChanged, statusLabel, &QLabel::setText);
				}
				abortListener->abort();
			});
	}
	return jobWidget;
}

void iAJobListView::newJobSlot()
{
	auto j = m_pendingJobs.pop();
	QString jobName = j->name;
	auto jobWidget = addJobWidget(jobName, j->prog, j->abrt, j->est);
	LOG(lvlDebug, QString("Job added: %1").arg(jobName));
	connect(j->obj, &QObject::destroyed, [this, jobWidget, jobName]()
	{
		LOG(lvlDebug, QString("Job '%1': Done.").arg(jobName));
		int oldJobCount = m_runningJobs.fetchAndAddOrdered(-1);
		if (oldJobCount == 1)
		{
			emit allJobsDone();
		}
		m_estimators.remove(jobWidget);
		jobWidget->deleteLater();
	});
	emit jobAdded();
}

void iAJobListView::addJob(QString name, iAProgress* p, QObject* t, iAAbortListener* abortListener,
	QSharedPointer<iADurationEstimator> estimator)
{
	m_runningJobs.fetchAndAddOrdered(1);
	m_pendingJobs.push(QSharedPointer<iAJobPending>::create(name, p, t, abortListener, estimator));
	emit newJobSignal();
}

QSharedPointer<QObject> iAJobListView::addJob(QString name, iAProgress* p,
	iAAbortListener* abortListener,	QSharedPointer<iADurationEstimator> estimator)
{
	QSharedPointer<QObject> result(new QObject);
	addJob(name, p, result.data(), abortListener, estimator);
	return result;
}



iADurationEstimator::~iADurationEstimator()
{
}