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
#include "dlg_imageproperty.h"

#include "iAImageInfo.h"

#include <vtkImageData.h>

dlg_imageproperty::dlg_imageproperty(QWidget *parent) : QDockWidget(parent)
{
	setupUi(this);
}

void dlg_imageproperty::EnterMsg(QString txt)
{
	lWidget->addItem(txt);
}

void dlg_imageproperty::Clear()
{
	lWidget->clear();
}

void dlg_imageproperty::AddInfo(vtkImageData* src, iAImageInfo const & info, QString const & name, int channelCount)
{
	EnterMsg(name);
	EnterMsg( QString( "    %1: [%2 %3]  [%4 %5]  [%6 %7]" )
		.arg(tr("Extent"))
		.arg( src->GetExtent()[0] )
		.arg( src->GetExtent()[1] )
		.arg( src->GetExtent()[2] )
		.arg( src->GetExtent()[3] )
		.arg( src->GetExtent()[4] )
		.arg( src->GetExtent()[5] ) );

	EnterMsg( QString( "    %1:  %2  %3  %4" )
		.arg(tr("Spacing"))
		.arg( src->GetSpacing()[0] )
		.arg( src->GetSpacing()[1] )
		.arg( src->GetSpacing()[2] ) );

	EnterMsg( QString( "    %1: %2 %3 %4" )
		.arg(tr("Origin"))
		.arg( src->GetOrigin()[0] )
		.arg( src->GetOrigin()[1] )
		.arg( src->GetOrigin()[2] ) );

	EnterMsg( QString("    %1: %2")
		.arg(tr("Datatype"))
		.arg(src->GetScalarTypeAsString()) );

	QString componentStr;
	if (src->GetNumberOfScalarComponents() > 1 && channelCount > 1)
	{
		componentStr = QString("%1/%2")
			.arg(channelCount)
			.arg(src->GetNumberOfScalarComponents());
	}
	else if (channelCount > 1)
	{
		componentStr = QString::number(channelCount);
	}
	else
	{
		componentStr = QString::number(src->GetNumberOfScalarComponents());
	}
	EnterMsg( QString( "    %1: %2" )
		.arg(tr("Components"))
		.arg(componentStr) );

	if ( src->GetNumberOfScalarComponents() == 1 ) //No histogram statistics for rgb, rgba or vector pixel type images
	{
		if (info.computing())
			EnterMsg("    Statistics are currently computing...");
		else if (info.voxelCount() == 0)
			EnterMsg("    Statistics not computed yet. Activate modality (by clicking on it) to do so.");
		else
			EnterMsg(tr("    VoxelCount: %1;  Min: %2;  Max: %3;  Mean: %4;  StdDev: %5;")
				.arg(info.voxelCount())
				.arg(info.min()).arg(info.max())
				.arg(info.mean()).arg(info.standardDeviation()));
	}
	lWidget->scrollToBottom();
	this->show();
}
