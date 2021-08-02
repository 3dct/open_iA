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

#include "iASignallingWidget.h"
#include "iAQVTKWidget.h"

class iAColoredWidget;

//! Keeps the aspect ratio of a contained iAQVTKWidget fixed
//! by placing two other resizable widgets around it as padding.
class iAFixedAspectWidget: public iASignallingWidget
{
	Q_OBJECT
public:
	iAFixedAspectWidget(double aspect=1.0, Qt::Alignment verticalAlign = Qt::AlignVCenter);
	iAQVTKWidget* vtkWidget();
	void setBGRole(QPalette::ColorRole role);
private:
	iAQVTKWidget* m_widget;
	iAColoredWidget* m_fill1, * m_fill2;
};