/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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
 
#include "pch.h"
#include "dlg_profile.h"

#include "charts/iAProfileWidget.h"

#include <QGridLayout>

dlg_profile::dlg_profile(QWidget *parent, vtkPolyData* profData, double rayLength ): QDockWidget (parent)
{
	setupUi(this);
	QString yCapt = "Greyvalue";
	QString xCapt = "Distance";
	profileWidget = new iAProfileWidget(profileAreaWidget, profData, rayLength, yCapt, xCapt);
	layout = new QGridLayout(profileAreaWidget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setObjectName(QString::fromUtf8("ProfileLayout"));
	layout->addWidget(profileWidget);
}

dlg_profile::~dlg_profile()
{
	delete profileWidget;
}

void dlg_profile::resizeEvent( QResizeEvent *event )
{
	QWidget::resizeEvent(event);
}