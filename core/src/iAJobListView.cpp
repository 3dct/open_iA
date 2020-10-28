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
#include "iAConsole.h"
#include "iAProgress.h"

//#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

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
}

QWidget* iAJobListView::addJobWidget(QString name, iAProgress* p, iAAbortListener* abortListener)
{
	auto titleLabel = new QLabel(name);
	titleLabel->setProperty("qssClass", "titleLabel");

	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	progressBar->setValue(0);

	auto statusLabel = new QLabel("");

	auto statusWidget = new QWidget();
	statusWidget->setLayout(new QVBoxLayout);
	statusWidget->layout()->setContentsMargins(0, 0, 0, 0);
	statusWidget->layout()->setSpacing(2);
	statusWidget->layout()->addWidget(statusLabel);
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

	// connections
	if (p)
	{
		connect(p, &iAProgress::progress, progressBar, &QProgressBar::setValue);
		connect(p, &iAProgress::statusChanged, statusLabel, &QLabel::setText);
	}
	if (abortListener)
	{
		connect(abortButton, &QToolButton::clicked, [=]()
			{
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