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

#include "iABoneThicknessChartXY.h"

#include <vtkAxis.h>
#include <vtkContextMouseEvent.h>
#include <vtkVector.h>

#include "iABoneThickness.h"
#include "iABoneThicknessTable.h"

iABoneThicknessChartXY::iABoneThicknessChartXY() : vtkChartXY ()
{
	ForceAxesToBoundsOn();
	SetClickActionToButton(vtkChart::NOTIFY, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::PAN, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::SELECT, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::SELECT_RECTANGLE, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::ZOOM, vtkContextMouseEvent::NO_BUTTON);
	SetClickActionToButton(vtkChart::ZOOM_AXIS, vtkContextMouseEvent::NO_BUTTON);
	ZoomWithMouseWheelOff();
}

bool iABoneThicknessChartXY::MouseButtonPressEvent(const vtkContextMouseEvent& e)
{
	if (e.GetButton() == vtkContextMouseEvent::LEFT_BUTTON)
	{
		const vtkVector2f v2dPos(e.GetPos());

		const int iSelected(selected(v2dPos.GetX()));

		if (m_pBoneThickness) m_pBoneThickness->setSelected(iSelected);
		if (m_pBoneThicknessTable) m_pBoneThicknessTable->setSelected(iSelected);
	}

	return vtkChartXY::MouseButtonPressEvent(e);
}

int iABoneThicknessChartXY::selected(const int& _iX)
{
	vtkAxis* pAxis(GetAxis(vtkAxis::BOTTOM));

	const double dAxisMinimum(pAxis->GetMinimum());
	const double dAxisMaximum(pAxis->GetMaximum());

	const float* pAxisPoint1(pAxis->GetPoint1());
	const float* pAxisPoint2(pAxis->GetPoint2());

	const double dAxisLeft((double)pAxisPoint1[0]);
	const double dAxisRight((double)pAxisPoint2[0]);

	const double dX(dAxisMinimum + (dAxisMaximum - dAxisMinimum) * ((double)_iX - dAxisLeft) / (dAxisRight - dAxisLeft));

	return (int) dX;
}

void iABoneThicknessChartXY::set(iABoneThickness* _pBoneThickness, iABoneThicknessTable* _pBoneThicknessTable)
{
	m_pBoneThickness = _pBoneThickness;
	m_pBoneThicknessTable = _pBoneThicknessTable;
}
