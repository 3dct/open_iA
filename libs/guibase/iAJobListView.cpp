// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAJobListView.h"

#include "iAAbortListener.h"
#include "iADurationEstimator.h"
#include "iALog.h"
#include "iAPerformanceHelper.h"    // for formatDuration
#include "iAProgress.h"

#include <QEventLoop>
#include <QLabel>
#include <QPainter>
#include <QProgressBar>
#include <QScrollArea>
#include <QSpacerItem>
#include <QTimer>
#include <QToolButton>
#include <QVariant>    // required for Linux build
#include <QVBoxLayout>

#include <chrono>
#include <mutex>

namespace
{
	std::mutex jobsMutex;
	std::mutex pendingJobsMutex;
}

//! Internal holder for data required about a currently running operation
class iAJob
{
public:
	iAJob(QString n, iAProgress* p, QObject* o, iAAbortListener* a, std::shared_ptr<iADurationEstimator> e) :
		name(n), progress(p), object(o), abortListener(a), estimator(e)
	{
	}

public:
	QString name;
	iAProgress* progress;
	QObject* object;
	iAAbortListener* abortListener;
	std::shared_ptr<iADurationEstimator> estimator;
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

//! Alternative for a QLabel which automatically cuts off text exceeding its width.
//! This widget, in contrast to a QLabel, is resizable to widths smaller than the required with to display its full text.
//! It does not impose any minimum width requirements on parent widgets, instead, any text text exceeding the width will
//! just be invisible.
class iAQShorteningLabel : public QScrollArea
{
public:
	iAQShorteningLabel(QString const& text, QString const & qssClass = QString()) : m_label(new QLabel(text))
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		setFrameShape(QFrame::NoFrame); // this to remove borders; background-color: transparent; in qss to make background transparent
		setWidgetResizable(true);
		setContentsMargins(0, 0, 0, 0);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setWidget(m_label);
		if (!qssClass.isEmpty())
		{
			m_label->setProperty("qssClass", qssClass);
		}
	}
public slots:
	void setText(QString const& text)
	{
		m_label->setText(text);
	}
private:
	QLabel* m_label;
};

iAJobListView* iAJobListView::get()
{
	static iAJobListView* jobList = new iAJobListView();
	return jobList;
}

iAJobListView::iAJobListView():
	m_insideLayout(new QVBoxLayout)
{

	auto insideWidget = new QWidget();
	insideWidget->setLayout(m_insideLayout);
	m_insideLayout->setContentsMargins(4, 4, 4, 4);
	m_insideLayout->setSpacing(4);
	m_insideLayout->addItem(new QSpacerItem(40, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

	auto scrollWidget = new QScrollArea();
	scrollWidget->setWidget(insideWidget);
	scrollWidget->setProperty("qssClass", "jobList");
	scrollWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollWidget->setWidgetResizable(true);

	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(0, 1, 0, 1);
	layout()->setSpacing(0);
	layout()->addWidget(scrollWidget);
	connect(this, &iAJobListView::newJobSignal, this, &iAJobListView::newJobSlot, Qt::QueuedConnection); // make sure widgets are created in GUI thread
}

iAJobListView::~iAJobListView()
{
	for (auto j : m_jobs)
	{
		if (j->abortListener)
		{
			j->abortListener->abort();
		}
	}
}

bool iAJobListView::isAnyJobRunning() const
{
	return !m_jobs.isEmpty() || !m_pendingJobs.isEmpty();
}

QWidget* iAJobListView::addJobWidget(std::shared_ptr<iAJob> j)
{
	{
		std::lock_guard<std::mutex> guard(jobsMutex);
		m_jobs.push_back(j);
	}
	auto titleLabel = new iAQShorteningLabel(j->name, "titleLabel");
	titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto progressBar = new QProgressBar();
	progressBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	progressBar->setRange(0, 1000);
	progressBar->setValue(0);

	auto statusLabel = new iAQShorteningLabel("");
	statusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	auto elapsedLabel = new iAQShorteningLabel("Elapsed: -");
	elapsedLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	auto remainingLabel = new iAQShorteningLabel("Remaining: unknown");
	remainingLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto timesWidget = new QWidget();
	auto timesLayout = new QHBoxLayout();
	timesWidget->setLayout(timesLayout);
	timesWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	timesLayout->setContentsMargins(0, 0, 0, 0);
	timesLayout->setSpacing(2);
	timesLayout->addWidget(elapsedLabel);
	timesLayout->addWidget(remainingLabel);

	auto statusWidget = new QWidget();
	auto statusLayout = new QVBoxLayout();
	statusWidget->setLayout(statusLayout);
	statusWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	statusLayout->setContentsMargins(0, 0, 0, 0);
	statusLayout->setSpacing(2);
	statusLayout->addWidget(titleLabel);
	statusLayout->addWidget(statusLabel);
	statusLabel->setVisible(false);
	statusLayout->addWidget(timesWidget);
	statusLayout->addWidget(progressBar);

	auto abortButton = new QToolButton();
	abortButton->setObjectName("pbAbort");
	abortButton->setEnabled(j->abortListener);

	auto jobWidget = new QWidget();
	jobWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	jobWidget->setProperty("qssClass", "jobWidget");
	auto jobVLayout = new QHBoxLayout();
	jobWidget->setLayout(jobVLayout);
	jobWidget->layout()->setContentsMargins(4, 4, 4, 4);
	jobWidget->layout()->setSpacing(4);
	jobWidget->layout()->addWidget(statusWidget);
	jobWidget->layout()->addWidget(abortButton);

	m_insideLayout->insertWidget(static_cast<int>(m_jobs.size()-1), jobWidget);

	if (!j->estimator)
	{
		j->estimator = std::make_shared<iAPercentBasedEstimator>();
	}
	QTimer* timer = new QTimer(jobWidget);
	connect(timer, &QTimer::timeout, jobWidget, [elapsedLabel, j] {
		elapsedLabel->setText(QString("Elapsed: %1").arg(formatDuration(j->estimator->elapsed(), false, true)));
	});
	timer->start(500);

	if (j->progress)
	{
		std::weak_ptr<iADurationEstimator> estim(j->estimator);	// reduce scope of captured var & don't capture shared pointer (-> mem leak)
		connect(j->progress, &iAProgress::progress, jobWidget, [progressBar, estim, remainingLabel](double value)
			{
				progressBar->setValue(value*10);
				auto estimator = estim.lock();
				double estRem = estimator ? estimator->estimatedTimeRemaining(value) : 0;
				remainingLabel->setText(
					QString("Remaining: %1").arg((estRem == -1) ? "unknown" : formatDuration(estRem, false, true)));
			});
		QString jobName(j->name);
		connect(j->progress, &iAProgress::statusChanged, jobWidget, [statusLabel, jobName](QString const& msg)
			{
				LOG(lvlDebug, QString("Job '%1': %2").arg(jobName).arg(msg));
				statusLabel->setVisible(true);
				statusLabel->setText(msg);
			});
	}
	if (j->abortListener)
	{
		connect(abortButton, &QToolButton::clicked, [=]()
			{
				LOG(lvlDebug, QString("Job '%1': Aborted.").arg(j->name));
				abortButton->setEnabled(false);
				statusLabel->setVisible(true);
				statusLabel->setText("Aborting...");
				if (j->progress)
				{
					QObject::disconnect(j->progress, &iAProgress::statusChanged, statusLabel, &iAQShorteningLabel::setText);
				}
				j->abortListener->abort();
			});
	}
	return jobWidget;
}

void iAJobListView::newJobSlot()
{
	std::shared_ptr<iAJob> j;
	{
		std::lock_guard<std::mutex> guard(pendingJobsMutex);
		j = m_pendingJobs.pop();
	}
	auto jobWidget = addJobWidget(j);
	LOG(lvlDebug, QString("Job started: %1.").arg(j->name));
	connect(j->object, &QObject::destroyed, [this, jobWidget, j]()
	{
		LOG(lvlDebug, QString("Job done: %1.").arg(j->name));
		qsizetype remainingJobs = 0;
		{
			std::lock_guard<std::mutex> guard(jobsMutex);
			auto idx = m_jobs.indexOf(j);
			if (idx != -1)
			{
				m_jobs.remove(idx);
			}
			else
			{
				LOG(lvlWarn, QString("Job '%1': Not found in list of running jobs!").arg(j->name));
			}
			remainingJobs = m_jobs.size();
		}
		if (remainingJobs == 0)
		{
			emit allJobsDone();
		}
		jobWidget->deleteLater();
	});
	emit jobAdded(j->object);
}

void iAJobListView::addJob(QString name, iAProgress* p, QObject* t, iAAbortListener* abortListener,
	std::shared_ptr<iADurationEstimator> estimator)
{
	{
		std::lock_guard<std::mutex> guard(pendingJobsMutex);
		m_pendingJobs.push(std::make_shared<iAJob>(name, p, t, abortListener, estimator));
	}
	emit newJobSignal();
	QEventLoop loop;
	connect(this, &iAJobListView::jobAdded, [&loop, t](QObject* currentT)
	{
		if (currentT == t)
		{
			loop.quit();
		}
	});
	loop.exec();
}

std::shared_ptr<QObject> iAJobListView::addJob(QString name, iAProgress* p,
	iAAbortListener* abortListener,	std::shared_ptr<iADurationEstimator> estimator)
{
	std::shared_ptr<QObject> result(new QObject);
	addJob(name, p, result.get(), abortListener, estimator);
	return result;
}



iADurationEstimator::~iADurationEstimator()
{
}



/*
// To debug job list layout, add this to some GUI initialization, e.g. to MainWindow constructor.

// requires includes:
#include "iAProgress.h"
#include "iARunAsync.h"

auto createSimpleJob = new QAction("Add job");
connect(createSimpleJob, &QAction::triggered, this, [this] {
		static int simpleJobCounter = 0;
		auto p = new iAProgress();
		auto fw = runAsync(
			[p]
			{
				const int N = 30;
				for (int i=0; i<N; ++i)
				{
					QThread::sleep(1);
					//p->setStatus(QString("Loop %1 / %2").arg(i).arg(N));
					p->emitProgress(100 * static_cast<double>(i) / N);
				}
			},
			[p] { delete p;
			}, this);
		iAJobListView::get()->addJob(QString("Simple Job %1").arg(simpleJobCounter++), p, fw);
});
m_ui->menuHelp->addAction(createSimpleJob);
auto createStatusJob = new QAction("Add job with status");
connect(createStatusJob, &QAction::triggered, this,
	[this]
	{
		static int statusJobCounter = 0;
		auto p = new iAProgress();
		auto fw = runAsync(
			[p]
			{
				const int N = 30;
				for (int i = 0; i < N; ++i)
				{
					QThread::sleep(1);
					p->setStatus(QString("Loop %1 / %2").arg(i).arg(N));
					p->emitProgress(100 * static_cast<double>(i) / N);
				}
			},
			[p] { delete p; }, this);
		iAJobListView::get()->addJob(QString("Status Job %1").arg(statusJobCounter++), p, fw);
	});
m_ui->menuHelp->addAction(createStatusJob);
*/
