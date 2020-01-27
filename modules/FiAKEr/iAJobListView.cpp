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

#include "iAConsole.h"
#include "iAProgress.h"

//#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <QVBoxLayout>



iAJobListView::iAJobListView(int margin)
{
	setLayout(new QVBoxLayout());
	layout()->setContentsMargins(margin, margin, margin, margin);
	layout()->setSpacing(margin);
}

void iAJobListView::addJob(QString name, iAProgress * p, QThread * t)
{
	auto jobWidget = new QWidget();
	jobWidget->setLayout(new QVBoxLayout());
	jobWidget->setStyleSheet("background-color:#EEE;");
	auto progressBar = new QProgressBar();
	progressBar->setRange(0, 100);
	progressBar->setValue(0);
	auto titleLabel = new QLabel(name);
	titleLabel->setStyleSheet("font-weight: bold;");
	jobWidget->layout()->addWidget(titleLabel);
	auto statusLabel = new QLabel("");
	jobWidget->layout()->addWidget(statusLabel);
	jobWidget->layout()->addWidget(progressBar);
	layout()->addWidget(jobWidget);
	jobWidget->setMaximumHeight(100);
	connect(p, &iAProgress::progress, progressBar, &QProgressBar::setValue);
	connect(p, &iAProgress::statusChanged, statusLabel, &QLabel::setText);
	connect(t, &QThread::finished, jobWidget, &QWidget::deleteLater);
}
