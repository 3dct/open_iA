/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAConsole.h"
#include "iAProgress.h"

//#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

iAJobListView::iAJobListView()
{
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(4, 4, 4, 4);
	layout()->setSpacing(4);
}

void iAJobListView::addJob(QString name, iAProgress * p, QThread * t, iAAbortListener* abortListener)
{
	m_runningJobs.fetchAndAddOrdered(1);
	auto titleLabel = new QLabel(name);
	titleLabel->setStyleSheet("font-weight: bold;");

	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	progressBar->setValue(0);

	auto statusLabel = new QLabel("");

	auto statusWidget = new QWidget();
	DEBUG_LOG(QString("Color: %1, colorname: %2")
		.arg(QWidget::palette().color(QPalette::Button).name() + ";"));
	statusWidget->setStyleSheet("background-color:" + QWidget::palette().color(QPalette::Button).name() + ";");
	statusWidget->setLayout(new QVBoxLayout);
	statusWidget->layout()->setContentsMargins(0, 0, 0, 0);
	statusWidget->layout()->setSpacing(4);
	statusWidget->layout()->addWidget(statusLabel);
	statusWidget->layout()->addWidget(progressBar);

	auto abortButton = new QPushButton("Abort");
	abortButton->setEnabled(abortListener);
	if (abortListener)
	{
		connect(abortButton, &QPushButton::clicked, [this, abortButton, abortListener, statusLabel, p]()
			{
				abortButton->setEnabled(false);
				statusLabel->setText("Aborting...");
				disconnect(p, &iAProgress::statusChanged, statusLabel, &QLabel::setText);
				abortListener->abort();
			});
	}

	auto contentWidget = new QWidget();
	contentWidget->setStyleSheet("background-color:#" + QWidget::palette().color(QPalette::Button).name() + ";");
	contentWidget->setLayout(new QHBoxLayout);
	contentWidget->layout()->setContentsMargins(0, 0, 0, 0);
	contentWidget->layout()->setSpacing(4);
	contentWidget->layout()->addWidget(statusWidget);
	contentWidget->layout()->addWidget(abortButton);

	auto jobWidget = new QWidget();
	jobWidget->setStyleSheet("background-color:#" + QWidget::palette().color(QPalette::Button).name() + ";");
	jobWidget->setLayout(new QVBoxLayout());
	jobWidget->setContentsMargins(4, 4, 4, 4);
	jobWidget->layout()->setSpacing(4);
	//jobWidget->setStyleSheet("background-color:#" + QWidget::palette().color(QPalette::Button).name() + ";");
	jobWidget->layout()->addWidget(titleLabel);
	jobWidget->layout()->addWidget(contentWidget);
	jobWidget->setMaximumHeight(150);
	connect(p, &iAProgress::progress, progressBar, &QProgressBar::setValue);
	connect(p, &iAProgress::statusChanged, statusLabel, &QLabel::setText);
	connect(t, &QThread::finished, [this, jobWidget]()
		{
			int oldJobCount = m_runningJobs.fetchAndAddOrdered(-1);
			if (oldJobCount == 1)
			{
				emit allJobsDone();
			}
			jobWidget->deleteLater();
		});
	//connect(t, &QThread::finished, jobWidget, &QWidget::deleteLater);

	layout()->addWidget(jobWidget);
}
