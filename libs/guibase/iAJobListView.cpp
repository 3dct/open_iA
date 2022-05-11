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

class iAJob
{
public:
	iAJob(QString n, iAProgress* p, QObject* o, iAAbortListener* a, QSharedPointer<iADurationEstimator> e) :
		name(n), progress(p), object(o), abortListener(a), estimator(e)
	{
	}

public:
	QString name;
	iAProgress* progress;
	QObject* object;
	iAAbortListener* abortListener;
	QSharedPointer<iADurationEstimator> estimator;
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

#define LABELOPT 2

#ifndef LABELOPT
using iAQShorteningLabel = QLabel;
#endif

#if LABELOPT == 1

// works half-way:
//     - height is not adapting properly (fm.height is about half of what it should be), 
//     - formatting (bold) doesn't work with drawText
class iAQShorteningLabel: public QWidget
{
public:
	iAQShorteningLabel(QString const& text, QString const& qssClass = QString()) :
		m_text(text),
		m_updateRequired(true),
		m_textHeight(10)
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		if (!qssClass.isEmpty())
		{
			setProperty("qssClass", qssClass);
		}
	}
	void paintEvent(QPaintEvent* /* ev*/) override
	{
		QPainter p(this);
		if (m_updateRequired)
		{
			updatePrintedText(p);
			m_updateRequired = false;
		}
		p.drawRect(QRect(2, 2, width()-4, height()-4));
		p.drawText(QRect(0, 0, width(), height()), Qt::AlignLeft, m_printedText);
	}
	void updatePrintedText(QPainter const & p)
	{
		QString ellipsis("..");
		int w = geometry().width();
		auto fm = p.fontMetrics();

		if (fm.horizontalAdvance(m_text) < w)
		{
			m_printedText = m_text;
		}
		else if (w <= fm.horizontalAdvance(ellipsis + "m"))  // space for ellipsis + at least one letter?
		{
			m_printedText = "";
		}
		else
		{
			// determine how much of the text can be printed:
			int len = m_text.length();
			bool found = false;
			int stepSize = len / 2;
			while (!found && stepSize > 0)
			{
				auto oldLen = len;
				if (fm.horizontalAdvance(m_text.left(len) + ellipsis) >= w)
				{
					len -= stepSize;
				}
				else
				{
					len += stepSize;
				}
				stepSize /= 2;
				// we want to find the length such that text[0..len] is within width, but text[0..len+1] isn't anymore
				found = fm.horizontalAdvance(m_text.left(len)     + ellipsis) < w &&
						fm.horizontalAdvance(m_text.left(len + 1) + ellipsis) >= w;
				LOG(lvlDebug,
					QString("oldLen=%1, newLen=%2, width=%3, found=%4")
						.arg(oldLen)
						.arg(len)
						.arg(fm.horizontalAdvance(m_text.left(len)))
						.arg(found));
			}
			if (!found)
			{
				LOG(lvlDebug, QString("NOT FOUND for text: %1 and width: %2!").arg(m_text).arg(w));
			}
			m_printedText = m_text.left(len) + ellipsis;
		}
		m_textHeight = fm.boundingRect(m_printedText).height();
	}
	void resizeEvent(QResizeEvent* /*ev*/)
	{
		m_updateRequired = true;
	}
	QSize sizeHint() const override
	{
		return QSize(20, m_textHeight);
	}
public slots:
	void setText(QString const& text)
	{
		m_text = text;
		m_updateRequired = true;
		update();
	}

private:
	QString m_text, m_printedText;
	int m_textHeight;
	bool m_updateRequired;
};

#elif LABELOPT == 2

// seems to work best, even though height distribution of widgets still not ideal:
//     title widget too high, other widgets too little height

class iAQShorteningLabel : public QScrollArea
{
public:
	iAQShorteningLabel(QString const& text, QString const & qssClass = QString()) : m_label(new QLabel(text))
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		//setAutoFillBackground(false);
		setFrameShape(QFrame::NoFrame);
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

#else
// does not help since initial setting of width already causes unwanted resize...
class iAQShorteningLabel : public QLabel
{
public:
	iAQShorteningLabel(QString const & text, QString const& qssClass = QString()): QLabel(text), settingText(false)
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		if (!qssClass.isEmpty())
		{
			setProperty("qssClass", qssClass);
		}
	}
	// cannot use setText since it isn't declared virtual...
	void mySetText(const QString & text)
	{
		myMax = maximumSize();
		myMin = minimumSize();
		setMinimumSize(size());
		setMaximumSize(size());
		settingText = true;

		QLabel::setText(text);
	}

	void resizeEvent(QResizeEvent *event) override
	{
		QLabel::resizeEvent(event);
		if(settingText){
			setMinimumSize(myMin);
			setMaximumSize(myMax);
			settingText = false;
		}
	}
private:
	QSize myMin, myMax;
	bool settingText;
};

#endif

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

QWidget* iAJobListView::addJobWidget(QSharedPointer<iAJob> j)
{
	{
		std::lock_guard<std::mutex> guard(jobsMutex);
		m_jobs.push_back(j);
	}
	auto titleLabel = new iAQShorteningLabel(j->name, "titleLabel");
	//titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 1000);
	progressBar->setValue(0);

	auto statusLabel = new iAQShorteningLabel("");
	auto elapsedLabel = new iAQShorteningLabel("Elapsed: -");
	auto remainingLabel = new iAQShorteningLabel("Remaining: unknown");

	auto timesLayout = new QHBoxLayout();
	timesLayout->setContentsMargins(0, 0, 0, 0);
	timesLayout->setSpacing(2);
	timesLayout->addWidget(elapsedLabel);
	timesLayout->addWidget(remainingLabel);

	auto statusWidget = new QWidget();
	//statusWidget->setMinimumHeight(20);
	auto statusLayout = new QVBoxLayout();
	statusWidget->setLayout(statusLayout);
	statusLayout->setContentsMargins(0, 0, 0, 0);
	statusLayout->setSpacing(2);
	statusLayout->addWidget(statusLabel);
	statusLayout->addLayout(timesLayout);
	statusLayout->addWidget(progressBar);

	auto abortButton = new QToolButton();
	abortButton->setIcon(QIcon(":/images/remove.png"));
	abortButton->setEnabled(j->abortListener);

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
	
	if (!j->estimator)
	{
		j->estimator = QSharedPointer<iAPercentBasedEstimator>::create();
	}
	QTimer* timer = new QTimer(jobWidget);
	connect(timer, &QTimer::timeout, jobWidget, [elapsedLabel, j] {
		elapsedLabel->setText(QString("Elapsed: %1").arg(formatDuration(j->estimator->elapsed(), false, true)));
	});
	timer->start(500);

	if (j->progress)
	{
		connect(j->progress, &iAProgress::progress, jobWidget, [progressBar, j, remainingLabel](double value)
			{
				progressBar->setValue(value*10);
				double estRem = j->estimator->estimatedTimeRemaining(value);
				remainingLabel->setText(
					QString("Remaining: %1").arg((estRem == -1) ? "unknown" : formatDuration(estRem, false, true)));
			});
		connect(j->progress, &iAProgress::statusChanged, [statusLabel, j](QString const& msg)
			{
				LOG(lvlDebug, QString("Job '%1': %2").arg(j->name).arg(msg));
				statusLabel->setText(msg);
			});
	}
	if (j->abortListener)
	{
		connect(abortButton, &QToolButton::clicked, [=]()
			{
				LOG(lvlDebug, QString("Job '%1': Aborted.").arg(j->name));
				abortButton->setEnabled(false);
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
	QSharedPointer<iAJob> j;
	{
		std::lock_guard<std::mutex> guard(pendingJobsMutex);
		j = m_pendingJobs.pop();
	}
	auto jobWidget = addJobWidget(j);
	LOG(lvlDebug, QString("Job started: %1.").arg(j->name));
	connect(j->object, &QObject::destroyed, [this, jobWidget, j]()
	{
		LOG(lvlDebug, QString("Job done: '%1'.").arg(j->name));
		int remainingJobs = 0;
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
	QSharedPointer<iADurationEstimator> estimator)
{
	{
		std::lock_guard<std::mutex> guard(pendingJobsMutex);
		m_pendingJobs.push(QSharedPointer<iAJob>::create(name, p, t, abortListener, estimator));
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
