/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "dlg_function.h"

#include <QMouseEvent>
#include <QPainter>

/*double dlg_function::v2dX(int x)
{
	double dataRange[2];
	histogram->getImageData()->GetScalarRange(dataRange);

	return ((double)(x-histogram->getTranslation()) / (double)histogram->geometry().width() * (dataRange[1] - dataRange[0]) ) /histogram->getZoom() + dataRange[0];
}

double dlg_function::v2dY(int y)
{
	return y / (double)(histogram->geometry().height() - histogram->getBottomMargin());
}

int dlg_function::d2vX(double x)
{
	double dataRange[2];
	histogram->getImageData()->GetScalarRange(dataRange);

	return (int)((x -dataRange[0]) * (double)histogram->geometry().width() / (dataRange[1] - dataRange[0])*histogram->getZoom());
}

int dlg_function::d2vY(double y)
{
	return (int)(y *(double)(histogram->geometry().height() - histogram->getBottomMargin()));
}*/
