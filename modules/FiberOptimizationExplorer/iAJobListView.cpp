/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAConsole.h"
#include "iAProgress.h"

//#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <QVBoxLayout>


iAJobData::iAJobData() : jobWidget(nullptr)
{}

void iAJobData::jobFinished()
{
	delete jobWidget;
	delete this;
}



iAJobListView::iAJobListView()
{
	setLayout(new QVBoxLayout());
}

void iAJobListView::addJob(QString name, iAProgress * p, QThread * t)
{
	iAJobData * jobdata = new iAJobData;
	jobdata->jobWidget = new QWidget();
	jobdata->jobWidget->setLayout(new QHBoxLayout());
	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	progressBar->setValue(0);
	jobdata->jobWidget->layout()->addWidget(new QLabel(name));
	jobdata->jobWidget->layout()->addWidget(progressBar);
	layout()->addWidget(jobdata->jobWidget);
	connect(p, &iAProgress::progress, progressBar, &QProgressBar::setValue);
	connect(t, &QThread::finished, jobdata, &iAJobData::jobFinished);
}
